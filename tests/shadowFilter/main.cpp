// Headless checks of the shadow filter math (ShadowFilter.h). No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/ShadowFilter.h"
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

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

  // Unit vectors including axis aligned ones and ones close to an axis
  std::vector<glm::vec3> lightDirections()
  {
    return {
      { 1.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f },
      { 0.0f, -1.0f, 0.0f },
      { -1.0f, 0.0f, 0.0f },
      glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)),
      glm::normalize(glm::vec3(0.4f, -1.0f, 0.3f)),
      glm::normalize(glm::vec3(1.0f, 0.001f, 0.0f)),
      glm::normalize(glm::vec3(0.001f, 1.0f, 0.002f)),
      glm::normalize(glm::vec3(0.0f, 0.0005f, -1.0f)),
      glm::normalize(glm::vec3(0.5f, 0.5f, 0.0f))
    };
  }

  void testDisc()
  {
    glm::vec2 sum(0.0f);
    float minimumDistance = std::numeric_limits<float>::max();
    bool allInside = true;

    for (int i = 0; i < vke::s_shadowFilterTaps; i++)
    {
      const glm::vec2 p = vke::s_shadowFilterDisc[static_cast<size_t>(i)];
      allInside = allInside && glm::length(p) <= 1.0f;
      sum += p;

      for (int j = i + 1; j < vke::s_shadowFilterTaps; j++)
      {
        minimumDistance = std::min(minimumDistance, glm::distance(p, vke::s_shadowFilterDisc[static_cast<size_t>(j)]));
      }
    }

    check(vke::s_shadowFilterTaps == 16, "there are 16 taps");
    check(allInside, "every disc point is inside the unit disc");
    check(glm::length(sum / static_cast<float>(vke::s_shadowFilterTaps)) < 0.02f, "the disc is centered on its origin");
    check(minimumDistance > 0.3f, "the disc points are well spread");
  }

  void testTapOffsets()
  {
    for (const glm::vec3& toLight : lightDirections())
    {
      bool perpendicular = true;
      bool lengthMatches = true;
      bool withinRadius = true;
      bool rotationKeepsLength = true;
      bool finite = true;

      for (int tap = 0; tap < vke::s_shadowFilterTaps; tap++)
      {
        const float radius = 2.5f;
        const float texel = 0.04f;
        const glm::vec3 offset = vke::shadowFilterTapOffset(tap, 0.0f, radius, texel, toLight);
        const float discLength = glm::length(vke::s_shadowFilterDisc[static_cast<size_t>(tap)]);

        finite = finite && std::isfinite(offset.x) && std::isfinite(offset.y) && std::isfinite(offset.z);
        perpendicular = perpendicular && std::abs(glm::dot(offset, toLight)) < 1e-6f;
        lengthMatches = lengthMatches && nearlyEqual(glm::length(offset), discLength * radius * texel, 1e-4f);
        withinRadius = withinRadius && glm::length(offset) <= radius * texel * (1.0f + 1e-5f);

        for (const float rotation : { 0.7f, 2.0f, 4.5f, 6.0f })
        {
          const glm::vec3 rotated = vke::shadowFilterTapOffset(tap, rotation, radius, texel, toLight);

          rotationKeepsLength = rotationKeepsLength && nearlyEqual(glm::length(rotated), glm::length(offset), 1e-4f) &&
                                std::abs(glm::dot(rotated, toLight)) < 1e-6f;
        }
      }

      check(finite, "tap offsets are finite for every light direction");
      check(perpendicular, "tap offsets are perpendicular to the light direction");
      check(lengthMatches, "an offset is its disc point's length times radius times texel");
      check(withinRadius, "an offset stays within radius times texel");
      check(rotationKeepsLength, "rotation keeps the length and stays in the plane");
    }

    // A rotation changes where the points land
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    check(glm::distance(vke::shadowFilterTapOffset(1, 0.0f, 1.0f, 1.0f, up), vke::shadowFilterTapOffset(1, 1.0f, 1.0f, 1.0f, up)) > 0.1f,
          "rotating moves the taps");
    // A quarter turn takes the disc point (x, y) to (-y, x) in the basis
    const glm::vec3 quarter = vke::shadowFilterTapOffset(4, 1.57079632679f, 1.0f, 1.0f, up);
    const glm::vec3 unrotated = vke::shadowFilterTapOffset(4, 0.0f, 1.0f, 1.0f, up);
    check(std::abs(glm::dot(quarter, unrotated)) < 1e-4f, "a quarter turn is perpendicular to the unrotated offset");
  }

  void testRadiusAndTexelScaling()
  {
    const glm::vec3 toLight = glm::normalize(glm::vec3(0.4f, -1.0f, 0.3f));

    bool zeroRadius = true;
    bool zeroTexel = true;
    bool radiusScales = true;
    bool texelScales = true;

    for (int tap = 0; tap < vke::s_shadowFilterTaps; tap++)
    {
      zeroRadius = zeroRadius && vke::shadowFilterTapOffset(tap, 1.3f, 0.0f, 0.05f, toLight) == glm::vec3(0.0f);
      zeroTexel = zeroTexel && vke::shadowFilterTapOffset(tap, 1.3f, 2.0f, 0.0f, toLight) == glm::vec3(0.0f);

      const glm::vec3 base = vke::shadowFilterTapOffset(tap, 1.3f, 1.0f, 0.05f, toLight);
      const glm::vec3 doubledRadius = vke::shadowFilterTapOffset(tap, 1.3f, 2.0f, 0.05f, toLight);
      const glm::vec3 doubledTexel = vke::shadowFilterTapOffset(tap, 1.3f, 1.0f, 0.1f, toLight);

      radiusScales = radiusScales && glm::distance(doubledRadius, base * 2.0f) < 1e-6f;
      texelScales = texelScales && glm::distance(doubledTexel, base * 2.0f) < 1e-6f;
    }

    check(zeroRadius, "a radius of 0 gives zero offsets");
    check(zeroTexel, "a texel of 0 gives zero offsets");
    check(radiusScales, "doubling the radius doubles the offsets");
    check(texelScales, "doubling the texel doubles the offsets");
  }

  void testNoise()
  {
    bool inRange = true;
    float minimum = 1.0f;
    float maximum = 0.0f;

    for (int y = 0; y < 64; y++)
    {
      for (int x = 0; x < 64; x++)
      {
        const float n = vke::interleavedGradientNoise({ static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f });
        inRange = inRange && n >= 0.0f && n < 1.0f;
        minimum = std::min(minimum, n);
        maximum = std::max(maximum, n);
      }
    }

    check(inRange, "the noise is in [0, 1)");
    check(minimum < 0.1f && maximum > 0.9f, "the noise covers the range");
    check(vke::interleavedGradientNoise({ 10.5f, 20.5f }) != vke::interleavedGradientNoise({ 11.5f, 20.5f }),
          "neighboring pixels get different values");
  }

  void testValidity()
  {
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    constexpr float inf = std::numeric_limits<float>::infinity();

    check(vke::isValidShadowFilterRadius(vke::s_defaultShadowFilterRadius), "the default is valid");
    check(vke::isValidShadowFilterRadius(0.0f), "0 is valid");
    check(vke::isValidShadowFilterRadius(8.0f), "a positive radius is valid");
    check(!vke::isValidShadowFilterRadius(-0.1f), "a negative radius is invalid");
    check(!vke::isValidShadowFilterRadius(nan), "a NaN radius is invalid");
    check(!vke::isValidShadowFilterRadius(inf), "an infinite radius is invalid");
    check(!vke::isValidShadowFilterRadius(-inf), "a negative infinite radius is invalid");
  }

  // Taps slid onto the receiver's plane: they lie in it, they moved only along the light (so they still land on the same
  // texel), a surface facing the light head-on leaves them alone, and the slide is clamped where the light grazes
  void testReceiverPlane()
  {
    const glm::vec3 toLight = glm::normalize(glm::vec3(0.3f, 1.0f, -0.2f));

    bool inPlane = true;
    bool alongLight = true;

    for (const glm::vec3& tilt : { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.5f, 1.0f, 0.0f), glm::vec3(-0.2f, 1.0f, 0.7f) })
    {
      const glm::vec3 normal = glm::normalize(tilt);

      for (int tap = 0; tap < vke::s_shadowFilterTaps; ++tap)
      {
        const glm::vec3 offset = vke::shadowFilterTapOffset(tap, 0.7f, 1.5f, 0.1f, toLight);
        const glm::vec3 slid = vke::receiverPlaneTapOffset(offset, normal, toLight);
        const glm::vec3 moved = slid - offset;

        inPlane = inPlane && std::abs(glm::dot(slid, normal)) <= 1e-5f;
        alongLight = alongLight && glm::length(glm::cross(moved, toLight)) <= 1e-5f;
      }
    }

    check(inPlane, "a slid tap lies in the receiver's plane");
    check(alongLight, "a tap slides only along the light");

    bool unchanged = true;

    for (int tap = 0; tap < vke::s_shadowFilterTaps; ++tap)
    {
      const glm::vec3 offset = vke::shadowFilterTapOffset(tap, 1.3f, 2.0f, 0.05f, toLight);
      const glm::vec3 slid = vke::receiverPlaneTapOffset(offset, toLight, toLight);

      unchanged = unchanged && glm::length(slid - offset) <= 1e-6f;
    }

    check(unchanged, "a surface facing the light head-on leaves the taps where they are");

    // A floor the light barely grazes: the slide would be about 1000 times the offset without the clamp
    const glm::vec3 grazing = glm::normalize(glm::vec3(1.0f, 0.001f, 0.0f));
    const glm::vec3 floorNormal(0.0f, 1.0f, 0.0f);
    const glm::vec3 offset(0.0f, 0.1f, 0.0f);
    const glm::vec3 slid = vke::receiverPlaneTapOffset(offset, floorNormal, grazing);

    check(nearlyEqual(glm::length(slid - offset), vke::s_maxShadowBiasTangent * 0.1f, 1e-4f),
          "the slide is clamped to the slope bias's tangent times the offset where the light grazes");
  }

} // namespace

int main()
{
  testDisc();
  testTapOffsets();
  testReceiverPlane();
  testRadiusAndTexelScaling();
  testNoise();
  testValidity();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
