// Headless checks of the directional light's cascade fitting. No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/DirectionalShadow.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

  int g_checks = 0;
  int g_failures = 0;

  constexpr uint32_t mapSize = 2048;

  void check(const bool condition,
             const std::string& name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  // NDC xy and [0, 1] depth of a world point
  glm::vec3 project(const glm::mat4& viewProjection,
                    const glm::vec3 point)
  {
    const glm::vec4 clip = viewProjection * glm::vec4(point, 1.0f);

    return glm::vec3(clip) / clip.w;
  }

  bool finite(const glm::mat4& m)
  {
    for (int c = 0; c < 4; ++c)
    {
      for (int r = 0; r < 4; ++r)
      {
        if (!std::isfinite(m[c][r]))
        {
          return false;
        }
      }
    }

    return true;
  }

  bool nearlyEqual(const float a,
                   const float b,
                   const float relative)
  {
    return std::abs(a - b) <= relative * std::max(std::abs(a), std::abs(b));
  }

  struct Case {
    std::string name;
    vke::CascadeCamera camera;
  };

  vke::CascadeCamera makeCamera(const glm::mat4& view)
  {
    return { view, 60.0f, 16.0f / 9.0f, 0.1f };
  }

  // The camera at eye turned by yaw and pitch (degrees)
  glm::mat4 viewAt(const glm::vec3 eye,
                   const float yaw,
                   const float pitch)
  {
    glm::mat4 world = glm::translate(glm::mat4(1.0f), eye);
    world = glm::rotate(world, glm::radians(yaw), glm::vec3(0, 1, 0));
    world = glm::rotate(world, glm::radians(pitch), glm::vec3(1, 0, 0));

    return glm::inverse(world);
  }

  // The eight world-space corners of the camera frustum between two view depths, computed here independently of the fit
  std::array<glm::vec3, 8> sliceCorners(const vke::CascadeCamera& camera,
                                        const float sliceNear,
                                        const float sliceFar)
  {
    const glm::mat4 world = glm::inverse(camera.view);
    const float tanHalf = std::tan(glm::radians(camera.fieldOfViewDegrees) / 2.0f);
    std::array<glm::vec3, 8> corners;
    int n = 0;

    for (const float depth : { sliceNear, sliceFar })
    {
      for (const float sx : { -1.0f, 1.0f })
      {
        for (const float sy : { -1.0f, 1.0f })
        {
          const glm::vec4 viewPoint(sx * depth * tanHalf * camera.aspect, sy * depth * tanHalf, -depth, 1.0f);

          corners[n++] = glm::vec3(world * viewPoint);
        }
      }
    }

    return corners;
  }

  const std::vector<glm::vec3>& directions()
  {
    static const std::vector<glm::vec3> list = {
      glm::vec3(0, -1, 0),
      glm::vec3(0.4f, -1.0f, 0.3f),
      glm::vec3(1.0f, -0.2f, 0.5f),
      glm::vec3(-0.3f, 0.6f, 0.2f)
    };

    return list;
  }

  const std::vector<Case>& cameras()
  {
    static const std::vector<Case> list = {
      { "identity", makeCamera(glm::mat4(1.0f)) },
      { "rotated and translated", makeCamera(glm::lookAt(glm::vec3(10, 5, -20), glm::vec3(3, 0, 4), glm::vec3(0, 1, 0))) },
      { "looking down", makeCamera(viewAt(glm::vec3(-4, 30, 2), 25.0f, -80.0f)) }
    };

    return list;
  }

  constexpr uint32_t cascadeCount = 4;

  std::array<float, vke::s_maxShadowCascades + 1> defaultSplits()
  {
    return vke::cascadeSplitDistances(0.1f, 100.0f, cascadeCount, 0.75f);
  }

  void checkSplits()
  {
    for (const uint32_t count : { 1u, 2u, 4u })
    {
      for (const float lambda : { 0.0f, 0.5f, 0.75f, 1.0f })
      {
        const auto splits = vke::cascadeSplitDistances(0.1f, 200.0f, count, lambda);
        bool increasing = true;

        for (uint32_t i = 0; i < count; ++i)
        {
          increasing = increasing && splits[i] < splits[i + 1];
        }

        check(splits[0] == 0.1f, "splits start at the near plane");
        check(splits[count] == 200.0f, "splits end at the shadow distance");
        check(increasing, "splits strictly increase");
      }

      const auto uniform = vke::cascadeSplitDistances(0.1f, 200.0f, count, 0.0f);
      const auto logarithmic = vke::cascadeSplitDistances(0.1f, 200.0f, count, 1.0f);
      bool uniformOk = true;
      bool logarithmicOk = true;

      for (uint32_t i = 1; i < count; ++i)
      {
        const float fraction = static_cast<float>(i) / static_cast<float>(count);

        uniformOk = uniformOk && nearlyEqual(uniform[i], 0.1f + (200.0f - 0.1f) * fraction, 1e-5f);
        logarithmicOk = logarithmicOk && nearlyEqual(logarithmic[i], 0.1f * std::pow(200.0f / 0.1f, fraction), 1e-4f);
      }

      check(uniformOk, "lambda 0 gives uniform splits");
      check(logarithmicOk, "lambda 1 gives logarithmic splits");
    }

    // A mix lies between the two
    const auto uniform = vke::cascadeSplitDistances(0.1f, 100.0f, 4, 0.0f);
    const auto logarithmic = vke::cascadeSplitDistances(0.1f, 100.0f, 4, 1.0f);
    const auto mixed = vke::cascadeSplitDistances(0.1f, 100.0f, 4, 0.5f);

    check(nearlyEqual(mixed[2], 0.5f * (uniform[2] + logarithmic[2]), 1e-5f), "lambda 0.5 averages the two distributions");
  }

  void checkContainment()
  {
    const auto splits = defaultSplits();

    vke::AABB casters;
    casters.expand(glm::vec3(-50, -5, -50));
    casters.expand(glm::vec3(50, 20, 50));

    for (const Case& c : cameras())
    {
      for (const glm::vec3& direction : directions())
      {
        for (const vke::AABB& bounds : { vke::AABB{}, casters })
        {
          for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
          {
            const vke::ShadowCascade fitted = vke::fitShadowCascade(direction, c.camera, splits[cascade],
                                                                    splits[cascade + 1], mapSize, bounds);
            bool contained = true;

            for (const glm::vec3& corner : sliceCorners(c.camera, splits[cascade], splits[cascade + 1]))
            {
              const glm::vec3 p = project(fitted.viewProjection, corner);

              contained = contained && std::abs(p.x) <= 1.0f + 1e-4f && std::abs(p.y) <= 1.0f + 1e-4f &&
                          p.z >= -1e-4f && p.z <= 1.0f + 1e-4f;
            }

            check(contained, "every slice corner lands inside the cascade (" + c.name + ", cascade " +
                             std::to_string(cascade) + ", direction y " + std::to_string(direction.y) + ")");
          }
        }
      }
    }
  }

  // The box holds the whole bounding sphere of the slice, not just its corners, after its center is snapped to the texel
  // grid: the sphere's extremes along the light's right and up axes stay inside the map. The radius is the one the fit
  // starts from, rounded up to 1/16 as it is; without the padding of a texel, the snap would push one side out.
  void checkSphereContainment()
  {
    const auto splits = defaultSplits();

    for (const Case& c : cameras())
    {
      for (const glm::vec3& direction : directions())
      {
        const glm::mat4 lightView = vke::directionalLightView(direction);
        const glm::vec3 right(lightView[0][0], lightView[1][0], lightView[2][0]);
        const glm::vec3 up(lightView[0][1], lightView[1][1], lightView[2][1]);

        for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
        {
          const vke::ShadowCascade fitted = vke::fitShadowCascade(direction, c.camera, splits[cascade],
                                                                  splits[cascade + 1], mapSize, {});
          const vke::BoundingSphere sphere = vke::frustumSliceBoundingSphere(c.camera, splits[cascade],
                                                                             splits[cascade + 1]);
          const float radius = std::ceil(sphere.radius * 16.0f) / 16.0f;
          bool contained = true;

          for (const glm::vec3& axis : { right, -right, up, -up })
          {
            const glm::vec3 p = project(fitted.viewProjection, sphere.center + axis * radius);

            contained = contained && std::abs(p.x) <= 1.0f + 1e-5f && std::abs(p.y) <= 1.0f + 1e-5f;
          }

          check(contained, "the snapped box holds the slice's bounding sphere (" + c.name + ", cascade " +
                           std::to_string(cascade) + ", direction y " + std::to_string(direction.y) + ")");
        }
      }
    }
  }

  // 1 over the half size of the cascade's orthographic box, read from the matrix (the light view is a pure rotation)
  float inverseHalfSize(const glm::mat4& viewProjection)
  {
    return glm::length(glm::vec3(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0]));
  }

  void checkRotationStability()
  {
    const auto splits = defaultSplits();
    const glm::vec3 eye(7.0f, 3.0f, -9.0f);

    for (const glm::vec3& direction : directions())
    {
      for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
      {
        const float reference = inverseHalfSize(vke::fitShadowCascade(direction, makeCamera(viewAt(eye, 0.0f, 0.0f)),
                                                                      splits[cascade], splits[cascade + 1], mapSize, {})
                                                  .viewProjection);
        bool stable = true;

        for (const auto& [yaw, pitch] : { std::pair{ 17.0f, 0.0f }, std::pair{ 140.0f, -35.0f },
                                          std::pair{ -75.0f, 52.0f }, std::pair{ 200.0f, 10.0f } })
        {
          const float size = inverseHalfSize(vke::fitShadowCascade(direction, makeCamera(viewAt(eye, yaw, pitch)),
                                                                   splits[cascade], splits[cascade + 1], mapSize, {})
                                               .viewProjection);

          stable = stable && nearlyEqual(size, reference, 1e-5f);
        }

        check(stable, "the cascade size does not change as the camera turns (cascade " + std::to_string(cascade) + ")");
      }
    }
  }

  void checkTranslationSnapping()
  {
    const auto splits = defaultSplits();
    const glm::vec3 point(3.7f, 1.2f, -2.4f);

    for (const glm::vec3& direction : directions())
    {
      for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
      {
        const auto texelOf = [&](const glm::vec3& offset) {
          const glm::mat4 view = viewAt(glm::vec3(7.0f, 3.0f, -9.0f) + offset, 30.0f, -10.0f);
          const glm::vec3 p = project(vke::fitShadowCascade(direction, makeCamera(view), splits[cascade],
                                                            splits[cascade + 1], mapSize, {}).viewProjection, point);

          return glm::vec2(p) * 0.5f + 0.5f;
        };

        const glm::vec2 reference = texelOf(glm::vec3(0.0f)) * static_cast<float>(mapSize);
        bool snapped = true;

        for (const glm::vec3& offset : { glm::vec3(0.0137f, 0.0f, 0.0f), glm::vec3(0.3f, 0.21f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 0.777f), glm::vec3(1.234f, -0.5f, 0.321f),
                                         glm::vec3(-3.3f, 0.07f, 5.9f) })
        {
          const glm::vec2 moved = texelOf(offset) * static_cast<float>(mapSize);

          for (int axis = 0; axis < 2; ++axis)
          {
            // The point moved by a whole number of texels: the fractions agree (mod 1)
            const float difference = moved[axis] - reference[axis];

            snapped = snapped && std::abs(difference - std::round(difference)) < 1e-2f;
          }
        }

        check(snapped, "a fixed point moves by whole texels as the camera translates (cascade " +
                       std::to_string(cascade) + ")");
      }
    }
  }

  void checkCasterReach()
  {
    const auto splits = defaultSplits();
    const vke::CascadeCamera camera = makeCamera(glm::lookAt(glm::vec3(0, 1, 0), glm::vec3(0, 1, -10), glm::vec3(0, 1, 0)));
    const glm::vec3 direction(0, -1, 0);

    // Far above the slice, between it and the light
    vke::AABB caster;
    caster.expand(glm::vec3(-1, 500, -9));
    caster.expand(glm::vec3(1, 510, -7));

    for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
    {
      const glm::mat4 viewProjection = vke::fitShadowCascade(direction, camera, splits[cascade], splits[cascade + 1],
                                                             mapSize, caster).viewProjection;
      float nearest = 1.0f;
      bool inRange = true;

      for (const float x : { caster.min.x, caster.max.x })
      {
        for (const float y : { caster.min.y, caster.max.y })
        {
          for (const float z : { caster.min.z, caster.max.z })
          {
            const float depth = project(viewProjection, glm::vec3(x, y, z)).z;

            nearest = std::min(nearest, depth);
            inRange = inRange && depth >= -1e-4f && depth <= 1.0f + 1e-4f;
          }
        }
      }

      check(inRange, "a caster far toward the light has depth within [0, 1] (cascade " + std::to_string(cascade) + ")");
      check(std::abs(nearest) < 1e-4f, "the nearest caster corner is at depth 0 (cascade " + std::to_string(cascade) + ")");

      // Without casters, or with casters nearer than the sphere's far half, the slice's center is mid-range
      glm::vec3 center(0.0f);

      for (const glm::vec3& corner : sliceCorners(camera, splits[cascade], splits[cascade + 1]))
      {
        center += corner / 8.0f;
      }

      vke::AABB inside;
      inside.expand(center - glm::vec3(0.01f));
      inside.expand(center + glm::vec3(0.01f));

      for (const vke::AABB& bounds : { vke::AABB{}, inside })
      {
        const glm::mat4 sphereOnly = vke::fitShadowCascade(direction, camera, splits[cascade], splits[cascade + 1],
                                                           mapSize, bounds).viewProjection;

        check(std::abs(project(sphereOnly, center).z - 0.5f) < 1e-4f,
              "without a caster beyond the sphere the near depth is the sphere's (cascade " + std::to_string(cascade) + ")");
      }
    }
  }

  void checkBias()
  {
    const auto splits = defaultSplits();

    for (const Case& c : cameras())
    {
      for (const glm::vec3& direction : directions())
      {
        for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
        {
          const vke::ShadowCascade fitted = vke::fitShadowCascade(direction, c.camera, splits[cascade],
                                                                  splits[cascade + 1], mapSize, {});
          const glm::mat4& m = fitted.viewProjection;

          // Read back from the matrix: x scales a world distance by 1 / half size, depth by 1 / range
          const float texel = 2.0f / (static_cast<float>(mapSize) * inverseHalfSize(m));
          const float depthRange = 1.0f / glm::length(glm::vec3(m[0][2], m[1][2], m[2][2]));

          check(nearlyEqual(fitted.depthBias, 2.0f * texel / depthRange, 1e-4f),
                "the bias is 2 texels over the depth range (" + c.name + ", cascade " + std::to_string(cascade) + ")");
        }
      }
    }
  }

  void checkFinite()
  {
    const auto splits = defaultSplits();

    for (const Case& c : cameras())
    {
      for (const glm::vec3& direction : { glm::vec3(0, -1, 0), glm::vec3(0.001f, -1, 0), glm::vec3(0, 1, 0.001f),
                                          glm::vec3(0.4f, -1.0f, 0.3f) })
      {
        for (uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
        {
          const vke::ShadowCascade fitted = vke::fitShadowCascade(direction, c.camera, splits[cascade],
                                                                  splits[cascade + 1], mapSize, {});

          check(finite(fitted.viewProjection) && std::isfinite(fitted.depthBias) && fitted.depthBias > 0.0f,
                "the matrix and bias are finite (" + c.name + ")");
        }
      }
    }
  }

} // namespace

int main()
{
  checkSplits();
  checkContainment();
  checkSphereContainment();
  checkRotationStability();
  checkTranslationSnapping();
  checkCasterReach();
  checkBias();
  checkFinite();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
