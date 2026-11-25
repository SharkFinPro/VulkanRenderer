// Headless checks of the shadow bias math (ShadowBias.h). No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/ShadowBias.h"
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

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

  bool nearlyEqual(const float a,
                   const float b,
                   const float tolerance = 1e-5f)
  {
    return std::abs(a - b) <= tolerance * std::max(1.0f, std::max(std::abs(a), std::abs(b)));
  }

  bool nearlyEqual(const glm::vec3& a,
                   const glm::vec3& b,
                   const float tolerance = 1e-5f)
  {
    return nearlyEqual(a.x, b.x, tolerance) && nearlyEqual(a.y, b.y, tolerance) && nearlyEqual(a.z, b.z, tolerance);
  }

  // A unit vector at the given angle from the normal (0, 1, 0), tilted toward +x
  glm::vec3 lightAt(const float degrees)
  {
    return { std::sin(glm::radians(degrees)), std::cos(glm::radians(degrees)), 0.0f };
  }

  const glm::vec3 fragPos(1.0f, 2.0f, 3.0f);
  const glm::vec3 normal(0.0f, 1.0f, 0.0f);

  void testZeroBias()
  {
    for (const float degrees : { 0.0f, 30.0f, 80.0f, 89.9f })
    {
      check(vke::shadowLookupPosition(fragPos, normal, lightAt(degrees), 0.5f, glm::vec3(0.0f)) == fragPos,
            "zero bias leaves the position alone");
    }

    check(vke::shadowLookupPosition(fragPos, normal, lightAt(30.0f), 0.0f, glm::vec3(1.0f, 2.0f, 3.0f)) == fragPos,
          "a zero texel leaves the position alone");
  }

  void testNormalOffset()
  {
    constexpr float texel = 0.25f;
    const glm::vec3 toLight = lightAt(40.0f);

    const glm::vec3 moved = vke::shadowLookupPosition(fragPos, normal, toLight, texel, glm::vec3(0.0f, 0.0f, 3.0f));

    check(nearlyEqual(moved, fragPos + normal * (3.0f * texel)),
          "the normal offset moves normalOffset texels along the normal");
  }

  void testConstantHeadOn()
  {
    constexpr float texel = 0.5f;
    const glm::vec3 moved = vke::shadowLookupPosition(fragPos, normal, normal, texel, glm::vec3(2.0f, 5.0f, 0.0f));

    check(nearlyEqual(moved, fragPos + normal * (2.0f * texel)),
          "facing the light head on, only the constant moves it toward the light");
  }

  void testSlopeGrowsWithAngle()
  {
    constexpr float texel = 1.0f;
    const glm::vec3 bias(0.0f, 1.0f, 0.0f);

    // The toward-light part is texel * slope * tan(theta), along toLight
    for (const float degrees : { 10.0f, 30.0f, 45.0f, 60.0f })
    {
      const glm::vec3 toLight = lightAt(degrees);
      const glm::vec3 moved = vke::shadowLookupPosition(fragPos, normal, toLight, texel, bias);
      const float expected = std::tan(glm::radians(degrees));

      check(nearlyEqual(moved - fragPos, toLight * expected, 1e-4f),
            "the slope term is the tangent of the angle along the light");
    }

    float previous = 0.0f;
    bool growing = true;

    for (const float degrees : { 5.0f, 20.0f, 40.0f, 60.0f, 80.0f })
    {
      const float length = glm::length(vke::shadowLookupPosition(fragPos, normal, lightAt(degrees), texel, bias) - fragPos);

      growing = growing && length > previous;
      previous = length;
    }

    check(growing, "the slope term grows toward grazing angles");
  }

  void testSlopeClamp()
  {
    constexpr float texel = 2.0f;
    const glm::vec3 bias(0.0f, 1.5f, 0.0f);
    const float maxMove = texel * 1.5f * vke::s_maxShadowBiasTangent;

    check(vke::s_maxShadowBiasTangent == 10.0f, "the largest tangent is 10");

    for (const float degrees : { 89.0f, 89.9f, 90.0f })
    {
      const glm::vec3 moved = vke::shadowLookupPosition(fragPos, normal, lightAt(degrees), texel, bias);

      check(nearlyEqual(glm::length(moved - fragPos), maxMove, 1e-4f),
            "near grazing the slope term stops at the largest tangent");
    }

    // Facing away: the cosine clamps to 0, and the term stays at the clamp rather than growing without bound
    const glm::vec3 away = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));
    const glm::vec3 moved = vke::shadowLookupPosition(fragPos, normal, away, texel, bias);

    check(nearlyEqual(glm::length(moved - fragPos), maxMove, 1e-4f), "a light behind the surface stays at the clamp");
  }

  void testTexelScaling()
  {
    const glm::vec3 bias(1.0f, 0.75f, 2.0f);
    const glm::vec3 toLight = lightAt(50.0f);

    const glm::vec3 one = vke::shadowLookupPosition(fragPos, normal, toLight, 0.1f, bias) - fragPos;
    const glm::vec3 two = vke::shadowLookupPosition(fragPos, normal, toLight, 0.2f, bias) - fragPos;

    check(nearlyEqual(two, one * 2.0f), "doubling the texel doubles the offset");
    check(nearlyEqual(vke::shadowLookupPosition(fragPos, normal, toLight, 0.1f, bias * 2.0f) - fragPos, one * 2.0f),
          "doubling the bias doubles the offset");

    const glm::vec3 combined = vke::shadowLookupPosition(fragPos, normal, toLight, 0.1f, bias);
    const glm::vec3 sum = vke::shadowLookupPosition(fragPos, normal, toLight, 0.1f, glm::vec3(bias.x, bias.y, 0.0f)) +
                          vke::shadowLookupPosition(fragPos, normal, toLight, 0.1f, glm::vec3(0.0f, 0.0f, bias.z)) - fragPos;

    check(nearlyEqual(combined, sum), "the normal and light terms add");
  }

  void testTexelPerDepth()
  {
    check(nearlyEqual(vke::pointShadowTexelPerDepth(1024.0f), 0.001953125f), "a point face covers 2 / mapSize per unit of depth");
    check(nearlyEqual(vke::pointShadowTexelPerDepth(512.0f), 2.0f / 512.0f), "a smaller map has larger texels");
    check(nearlyEqual(vke::spotShadowTexelPerDepth(90.0f, 1024.0f), 2.0f / 1024.0f), "a 90 degree spot matches a point face");
    check(nearlyEqual(vke::spotShadowTexelPerDepth(60.0f, 1024.0f), 2.0f * std::tan(glm::radians(30.0f)) / 1024.0f),
          "a 60 degree spot covers 2 * tan(30) / mapSize");

    // 2 * 0.57735027 / 1024
    check(nearlyEqual(vke::spotShadowTexelPerDepth(60.0f, 1024.0f), 0.00112764f, 1e-4f), "a 60 degree spot, hand computed");
    check(vke::spotShadowTexelPerDepth(30.0f, 1024.0f) < vke::spotShadowTexelPerDepth(60.0f, 1024.0f),
          "a narrower cone has smaller texels");
  }

  void testValidity()
  {
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    constexpr float inf = std::numeric_limits<float>::infinity();

    check(vke::isValidShadowBias(vke::ShadowBias {}), "the defaults are valid");
    check(vke::isValidShadowBias({ 0.0f, 0.0f, 0.0f }), "all zero is valid");
    check(!vke::isValidShadowBias({ -0.1f, 1.0f, 1.0f }), "a negative constant is invalid");
    check(!vke::isValidShadowBias({ 1.0f, -0.1f, 1.0f }), "a negative slope is invalid");
    check(!vke::isValidShadowBias({ 1.0f, 1.0f, -0.1f }), "a negative normal offset is invalid");
    check(!vke::isValidShadowBias({ nan, 1.0f, 1.0f }), "a NaN constant is invalid");
    check(!vke::isValidShadowBias({ 1.0f, nan, 1.0f }), "a NaN slope is invalid");
    check(!vke::isValidShadowBias({ 1.0f, 1.0f, nan }), "a NaN normal offset is invalid");
    check(!vke::isValidShadowBias({ inf, 1.0f, 1.0f }), "an infinite constant is invalid");
    check(!vke::isValidShadowBias({ 1.0f, inf, 1.0f }), "an infinite slope is invalid");
    check(!vke::isValidShadowBias({ 1.0f, 1.0f, -inf }), "an infinite normal offset is invalid");
  }

} // namespace

int main()
{
  testZeroBias();
  testNormalOffset();
  testConstantHeadOn();
  testSlopeGrowsWithAngle();
  testSlopeClamp();
  testTexelScaling();
  testTexelPerDepth();
  testValidity();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
