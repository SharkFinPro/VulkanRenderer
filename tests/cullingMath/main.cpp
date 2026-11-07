// Headless checks of the culling math (AABB, transformAABB, Frustum). No engine, no GPU: ctest runs this.
#include "source/utilities/Bounds.h"
#include "source/utilities/Frustum.h"
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const char* name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  bool approx(const glm::vec3& a,
              const glm::vec3& b,
              const float epsilon = 1e-4f)
  {
    return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec3(epsilon)));
  }

  vke::AABB boxAt(const glm::vec3 center,
                  const float half)
  {
    return { center - glm::vec3(half), center + glm::vec3(half) };
  }

  // The engine's camera projection: GLM perspective (depth 0..1) with the Y flip Vulkan needs.
  glm::mat4 cameraProjection(const bool flipY)
  {
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    if (flipY)
    {
      projection[1][1] *= -1;
    }

    return projection;
  }

  // Camera at the origin looking down +Z with +Y up. At z = 10 the visible half height is about 4.1 and the
  // half width about 7.4. World +X is to the camera's left.
  vke::Frustum cameraFrustum(const bool flipY)
  {
    const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return vke::Frustum::fromViewProjection(cameraProjection(flipY) * view);
  }

  void checkAabb()
  {
    const vke::AABB empty;
    check(empty.isEmpty(), "default AABB is empty");

    vke::AABB single;
    single.expand({ 1.0f, 2.0f, 3.0f });
    check(!single.isEmpty(), "AABB with one point is not empty");
    check(single.min == single.max, "AABB with one point has min == max");

    vke::AABB box;
    box.expand({ -1.0f, 0.0f, 2.0f });
    box.expand({ 3.0f, 4.0f, -2.0f });
    box.expand({ 0.0f, 1.0f, 0.0f });
    check(box.min == glm::vec3(-1.0f, 0.0f, -2.0f), "AABB min after several points");
    check(box.max == glm::vec3(3.0f, 4.0f, 2.0f), "AABB max after several points");
    check(approx(box.center(), { 1.0f, 2.0f, 0.0f }), "AABB center");
    check(approx(box.halfExtents(), { 2.0f, 2.0f, 2.0f }), "AABB half extents");
  }

  void checkTransformAgainstCorners();

  void checkTransform()
  {
    const vke::AABB unit = boxAt(glm::vec3(0.0f), 1.0f);

    const vke::AABB moved = vke::transformAABB(unit, glm::translate(glm::mat4(1.0f), { 5.0f, -2.0f, 1.0f }));
    check(approx(moved.center(), { 5.0f, -2.0f, 1.0f }), "translation moves the center");
    check(approx(moved.halfExtents(), { 1.0f, 1.0f, 1.0f }), "translation keeps the extents");

    const vke::AABB scaled = vke::transformAABB(unit, glm::scale(glm::mat4(1.0f), glm::vec3(3.0f)));
    check(approx(scaled.halfExtents(), { 3.0f, 3.0f, 3.0f }), "uniform scale scales the extents");

    // A unit cube (half extent 1) turned 45 degrees about Y spans sqrt(2) either side on X and Z; Y is unchanged.
    const vke::AABB rotated = vke::transformAABB(unit, glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), { 0.0f, 1.0f, 0.0f }));
    const float root2 = std::sqrt(2.0f);
    check(approx(rotated.halfExtents(), { root2, 1.0f, root2 }), "45 degree rotation about Y");

    const vke::AABB mirrored = vke::transformAABB(boxAt({ 1.0f, 0.0f, 0.0f }, 1.0f), glm::scale(glm::mat4(1.0f), { -2.0f, 1.0f, 1.0f }));
    check(!mirrored.isEmpty(), "negative scale keeps a valid box");
    check(approx(mirrored.min, { -4.0f, -1.0f, -1.0f }) && approx(mirrored.max, { 0.0f, 1.0f, 1.0f }), "negative scale bounds");

    check(vke::transformAABB(vke::AABB {}, glm::translate(glm::mat4(1.0f), { 1.0f, 1.0f, 1.0f })).isEmpty(), "empty stays empty");

    checkTransformAgainstCorners();
  }

  // An offset, non-cubic box under a non-symmetric transform, so transposed indexing would give different extents.
  void checkTransformAgainstCorners()
  {
    const vke::AABB box { { 0.0f, -2.0f, -3.0f }, { 2.0f, 2.0f, 3.0f } };

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 4.0f, 1.0f, -2.0f });
    transform = glm::rotate(transform, glm::radians(30.0f), { 1.0f, 0.0f, 0.0f });
    transform = glm::rotate(transform, glm::radians(50.0f), { 0.0f, 0.0f, 1.0f });
    transform = glm::scale(transform, { 1.0f, 0.5f, 2.0f });

    vke::AABB expected;
    for (int corner = 0; corner < 8; ++corner)
    {
      const glm::vec3 point {
        (corner & 1) != 0 ? box.max.x : box.min.x,
        (corner & 2) != 0 ? box.max.y : box.min.y,
        (corner & 4) != 0 ? box.max.z : box.min.z
      };
      expected.expand(glm::vec3(transform * glm::vec4(point, 1.0f)));
    }

    const vke::AABB transformed = vke::transformAABB(box, transform);
    check(approx(transformed.min, expected.min) && approx(transformed.max, expected.max),
          "rotated, scaled, offset box matches its transformed corners");
  }

  void checkPlanes()
  {
    for (const glm::vec4& plane : cameraFrustum(true).getPlanes())
    {
      check(std::abs(glm::length(glm::vec3(plane)) - 1.0f) < 1e-4f, "plane normal has unit length");
    }
  }

  void checkFrustumBoxes(const bool flipY)
  {
    const vke::Frustum frustum = cameraFrustum(flipY);
    const std::string suffix = flipY ? " (flipped Y)" : " (plain Y)";
    const auto expect = [&suffix](const bool actual, const bool expected, const char* name) {
      check(actual == expected, (std::string(name) + suffix).c_str());
    };

    expect(frustum.intersects(boxAt({ 0.0f, 0.0f, 10.0f }, 1.0f)), true, "box ahead is visible");
    expect(frustum.intersects(boxAt({ 0.0f, 0.0f, -10.0f }, 1.0f)), false, "box behind the camera");
    expect(frustum.intersects(boxAt({ 0.0f, 0.0f, 150.0f }, 1.0f)), false, "box beyond the far plane");
    // Between the camera and the near plane (0.1): z in [0.06, 0.09].
    // The OpenGL form of the near plane would sit at 0.05 and accept it.
    expect(frustum.intersects(boxAt({ 0.0f, 0.0f, 0.075f }, 0.015f)), false, "box nearer than the near plane");
    expect(frustum.intersects(boxAt({ 50.0f, 0.0f, 10.0f }, 1.0f)), false, "box far to the left");
    expect(frustum.intersects(boxAt({ -50.0f, 0.0f, 10.0f }, 1.0f)), false, "box far to the right");
    expect(frustum.intersects(boxAt({ 0.0f, 50.0f, 10.0f }, 1.0f)), false, "box far above");
    expect(frustum.intersects(boxAt({ 0.0f, -50.0f, 10.0f }, 1.0f)), false, "box far below");
    // The side planes sit at x = +-7.36 at z = 10, so a box centered on one straddles it.
    expect(frustum.intersects(boxAt({ 7.36f, 0.0f, 10.0f }, 1.0f)), true, "box straddling the left plane");
    expect(frustum.intersects(boxAt({ 0.0f, 0.0f, 0.0f }, 1000.0f)), true, "huge box containing the camera");
    expect(frustum.intersects(vke::AABB {}), false, "empty box");
  }

  void checkFrustumSpheres()
  {
    const vke::Frustum frustum = cameraFrustum(true);

    check(frustum.intersects({ 0.0f, 0.0f, 10.0f }, 1.0f), "sphere ahead is visible");
    check(!frustum.intersects({ 0.0f, 0.0f, -10.0f }, 1.0f), "small sphere behind the camera");
    // Center 0.5 behind the camera: the radius reaches past the near plane and the side planes.
    check(frustum.intersects({ 0.0f, 0.0f, -0.5f }, 1.0f), "sphere behind but reaching into the frustum");
  }

  // Mirrors SpotLight::getLightViewProjectionMatrix: a light at (0, 5, 0) shining straight down (so the up
  // vector is +X), a 30 degree cone (60 degree field of view), square aspect, Y flipped.
  void checkSpotLight()
  {
    const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 4.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
    projection[1][1] *= -1;
    const vke::Frustum frustum = vke::Frustum::fromViewProjection(projection * view);

    check(frustum.intersects(boxAt({ 0.0f, -5.0f, 0.0f }, 1.0f)), "spot light keeps a box in the cone");
    check(!frustum.intersects(boxAt({ 0.0f, 10.0f, 0.0f }, 1.0f)), "spot light culls a box behind it");
    check(!frustum.intersects(boxAt({ 50.0f, -5.0f, 0.0f }, 1.0f)), "spot light culls a box outside the cone");
  }

} // namespace

int main()
{
  checkAabb();
  checkTransform();
  checkPlanes();
  checkFrustumBoxes(true);
  checkFrustumBoxes(false);
  checkFrustumSpheres();
  checkSpotLight();

  if (g_failures > 0)
  {
    std::cout << g_failures << " of " << g_checks << " checks failed\n";
    return EXIT_FAILURE;
  }

  std::cout << g_checks << " checks passed\n";
  return EXIT_SUCCESS;
}
