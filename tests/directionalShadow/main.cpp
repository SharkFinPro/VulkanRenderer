// Headless checks of the directional light's shadow fitting. No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/DirectionalShadow.h"
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

  int g_checks = 0;
  int g_failures = 0;

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

  // Any vector perpendicular to the direction
  glm::vec3 perpendicular(const glm::vec3 direction)
  {
    const glm::vec3 axis = std::abs(direction.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

    return glm::normalize(glm::cross(direction, axis));
  }

  constexpr float epsilon = 1e-5f;

  void checkDirection(const glm::vec3 rawDirection,
                      const glm::vec3 center,
                      const float halfExtent)
  {
    const glm::vec3 direction = glm::normalize(rawDirection);
    const std::string label = "direction (" + std::to_string(direction.x) + ", " + std::to_string(direction.y) + ", " +
                              std::to_string(direction.z) + ")";
    const glm::mat4 viewProjection = vke::directionalShadowViewProjection(direction, center, halfExtent);

    check(finite(viewProjection), label + ": the matrix is finite");

    const glm::vec3 centerNdc = project(viewProjection, center);
    check(std::abs(centerNdc.x) < epsilon && std::abs(centerNdc.y) < epsilon, label + ": the center maps to NDC (0, 0)");
    check(std::abs(centerNdc.z - 0.5f) < epsilon, label + ": the center maps to depth 0.5");

    check(std::abs(project(viewProjection, center - direction * halfExtent).z) < epsilon,
          label + ": the light-facing side maps to depth 0");
    check(std::abs(project(viewProjection, center + direction * halfExtent).z - 1.0f) < epsilon,
          label + ": the far side maps to depth 1");

    // Strictly increasing along the direction
    float previous = project(viewProjection, center - direction * halfExtent).z;
    bool increasing = true;
    for (int i = 1; i <= 8; ++i)
    {
      const float t = -1.0f + 2.0f * static_cast<float>(i) / 8.0f;
      const float depth = project(viewProjection, center + direction * (t * halfExtent)).z;
      increasing = increasing && depth > previous;
      previous = depth;
    }
    check(increasing, label + ": depth increases along the direction");

    // Sideways, the map is as wide as the half extent in every direction around the light's axis
    const glm::vec3 side = perpendicular(direction);
    const glm::vec3 other = glm::cross(direction, side);

    for (const glm::vec3 axis : { side, -side, other, -other, glm::normalize(side + other), glm::normalize(side - other) })
    {
      const glm::vec3 inside = project(viewProjection, center + axis * (halfExtent * 0.99f));
      const glm::vec3 outside = project(viewProjection, center + axis * (halfExtent * 1.01f));

      check(glm::length(glm::vec2(inside)) < 1.0f, label + ": 0.99 of the half extent is inside the map");
      check(glm::length(glm::vec2(outside)) > 1.0f, label + ": 1.01 of the half extent is outside the map");
    }
  }

} // namespace

int main()
{
  const std::array directions = {
    glm::vec3(0, -1, 0),
    glm::vec3(0, 1, 0),
    glm::vec3(0.4f, -1.0f, 0.3f),
    glm::vec3(1, 0, 0),
    glm::vec3(0.2f, 0.1f, -1.0f)
  };

  for (const glm::vec3 direction : directions)
  {
    checkDirection(direction, glm::vec3(0.0f), 50.0f);
    checkDirection(direction, glm::vec3(10.0f, -3.0f, 25.0f), 7.5f);
  }

  // Near vertical, where the up vector switches
  for (const glm::vec3 direction : { glm::vec3(0, -1, 0), glm::vec3(0.001f, -1, 0), glm::vec3(0, -1, 0.001f) })
  {
    check(finite(vke::directionalShadowViewProjection(direction, glm::vec3(0.0f), 50.0f)),
          "the matrix is finite for a near-vertical direction");
  }

  check(std::abs(vke::directionalShadowBias(2048) - 2.0f / 2048.0f) < 1e-9f, "the bias is 2 / mapSize");
  check(std::abs(vke::directionalShadowBias(1024) - 2.0f / 1024.0f) < 1e-9f, "the bias is 2 / mapSize at 1024");

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
