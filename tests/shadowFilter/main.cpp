// Headless checks of the shadow filter math (ShadowFilter.h). No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/ShadowFilter.h"
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
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
    check(vke::isValidShadowFilterRadius(vke::s_maxShadowFilterRadius), "the widest radius is valid");
    check(!vke::isValidShadowFilterRadius(vke::s_maxShadowFilterRadius + 0.5f), "a radius past the widest is invalid");
    check(!vke::isValidShadowFilterRadius(1e30f), "a huge finite radius is invalid");
  }

  // The shader holds its own copies of the disc, the noise constants and the slide's clamp; read them out of the source
  // so the C++ mirror can't drift from what the GPU runs
  void testShaderMatchesMirror()
  {
    std::ifstream file(VKE_SCENE_LIGHTING_GLSL);
    check(file.good(), "SceneLighting.glsl can be read");

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string source = buffer.str();

    const size_t discStart = source.find("shadowFilterDisc[shadowFilterTaps] = vec2[shadowFilterTaps](");
    const size_t discEnd = source.find(");", discStart);
    check(discStart != std::string::npos && discEnd != std::string::npos, "the shader declares the disc");

    if (discStart != std::string::npos && discEnd != std::string::npos)
    {
      const std::string disc = source.substr(discStart, discEnd - discStart);
      const std::regex point(R"(vec2\(\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*\))");

      int count = 0;
      bool same = true;

      for (auto it = std::sregex_iterator(disc.begin(), disc.end(), point); it != std::sregex_iterator(); ++it)
      {
        if (count < vke::s_shadowFilterTaps)
        {
          const glm::vec2 expected = vke::s_shadowFilterDisc[static_cast<size_t>(count)];
          same = same && std::abs(std::stof((*it)[1].str()) - expected.x) <= 1e-6f &&
                 std::abs(std::stof((*it)[2].str()) - expected.y) <= 1e-6f;
        }

        ++count;
      }

      check(count == vke::s_shadowFilterTaps, "the shader's disc has as many points as the mirror's");
      check(same, "the shader's disc points equal the mirror's, in order");
    }

    check(source.find("const int shadowFilterTaps = 16;") != std::string::npos && vke::s_shadowFilterTaps == 16,
          "the shader takes as many taps as the mirror");
    check(source.find("52.9829189 * fract(dot(fragCoord, vec2(0.06711056, 0.00583715)))") != std::string::npos,
          "the shader's noise uses the mirror's constants");

    const std::regex tangent(R"(const float maxTangent = ([0-9.]+);)");
    bool tangentsMatch = true;
    int tangents = 0;

    for (auto it = std::sregex_iterator(source.begin(), source.end(), tangent); it != std::sregex_iterator(); ++it)
    {
      tangentsMatch = tangentsMatch && std::stof((*it)[1].str()) == vke::s_maxShadowBiasTangent;
      ++tangents;
    }

    check(tangents > 0 && tangentsMatch, "the shader clamps the slide with the mirror's tangent");

    check(source.find("const ivec2 shadowFilterEarlyTaps = ivec2(" + std::to_string(vke::s_shadowFilterFirstEarlyTap) +
                      ", " + std::to_string(vke::s_shadowFilterLastEarlyTap) + ");") != std::string::npos,
          "the shader takes the mirror's early taps first");
  }

  // The early taps stand in for the whole disc where they agree, so they must be four points on its rim, spread around it:
  // no gap between neighbors wider than a third of the circle
  void testEarlyTaps()
  {
    check(vke::s_shadowFilterLastEarlyTap - vke::s_shadowFilterFirstEarlyTap + 1 == 4 &&
          vke::s_shadowFilterFirstEarlyTap >= 0 && vke::s_shadowFilterLastEarlyTap < vke::s_shadowFilterTaps,
          "there are four early taps, all in the disc");

    std::vector<float> angles;
    bool onRim = true;

    for (int tap = vke::s_shadowFilterFirstEarlyTap; tap <= vke::s_shadowFilterLastEarlyTap; ++tap)
    {
      const glm::vec2 d = vke::s_shadowFilterDisc[static_cast<size_t>(tap)];
      onRim = onRim && glm::length(d) > 0.85f;
      angles.push_back(std::atan2(d.y, d.x));
    }

    std::sort(angles.begin(), angles.end());

    float widestGap = 0.0f;

    for (size_t k = 0; k < angles.size(); ++k)
    {
      const float next = k + 1 < angles.size() ? angles[k + 1] : angles.front() + 2.0f * 3.14159265f;
      widestGap = std::max(widestGap, next - angles[k]);
    }

    check(onRim, "the early taps lie on the disc's rim");
    check(widestGap < 2.0f * 3.14159265f / 3.0f, "the early taps spread around the rim");
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

    // A normal facing away from the light (callers never pass one, but the floor and the clamp must still hold)
    const glm::vec3 away = vke::receiverPlaneTapOffset(glm::vec3(0.05f, 0.0f, 0.02f), glm::vec3(0.0f, -1.0f, 0.0f),
                                                       glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f)));
    check(std::isfinite(away.x) && std::isfinite(away.y) && std::isfinite(away.z) &&
          glm::length(away - glm::vec3(0.05f, 0.0f, 0.02f)) <= vke::s_maxShadowBiasTangent * glm::length(glm::vec3(0.05f, 0.0f, 0.02f)) * 1.0001f,
          "a normal facing away from the light gives a finite, clamped slide");
  }

  // The shader's per-lookup frame gives the same taps as the per-tap reference, slid and clamped alike
  void testFactoredFrame()
  {
    bool same = true;

    for (const glm::vec3& toLight : lightDirections())
    {
      // The basis shadowFilterTapOffset builds: rotation 0 and unit scale put disc points (1, 0) and (0, 1) on t and b
      const glm::vec3 absLight = glm::abs(toLight);
      const glm::vec3 helper = absLight.x <= absLight.y && absLight.x <= absLight.z ? glm::vec3(1.0f, 0.0f, 0.0f)
                               : absLight.y <= absLight.z ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                          : glm::vec3(0.0f, 0.0f, 1.0f);
      const glm::vec3 t = glm::normalize(glm::cross(toLight, helper));
      const glm::vec3 b = glm::cross(toLight, t);

      // Head-on, tilted and grazing (clamped). Not facing away: callers skip the lookup there (facesLight), and both forms
      // divide a dot product that is 0 up to rounding by the 1e-4 floor, so they agree only to that rounding times 1e4
      // (testReceiverPlane checks such a slide stays finite and clamped).
      const glm::vec3 grazing = glm::normalize(glm::cross(toLight, t) + toLight * 0.001f);

      for (const glm::vec3& normal : { toLight, glm::normalize(toLight + t * 0.8f), grazing })
      {
        for (const float rotation : { 0.0f, 0.7f, 2.9f, 5.5f })
        {
          const float radius = 1.5f;
          const float texel = 0.04f;
          const vke::ShadowFilterFrame frame = vke::shadowFilterFrame(glm::vec2(std::cos(rotation), std::sin(rotation)),
                                                                      t, b, radius * texel, normal, toLight);

          for (int tap = 0; tap < vke::s_shadowFilterTaps; ++tap)
          {
            const glm::vec3 reference = vke::receiverPlaneTapOffset(
              vke::shadowFilterTapOffset(tap, rotation, radius, texel, toLight), normal, toLight);
            const glm::vec3 factored = vke::shadowFilterFrameOffset(tap, frame);
            const float size = std::max(glm::length(reference), radius * texel);

            same = same && glm::length(factored - reference) <= 1e-4f * size;
          }
        }
      }
    }

    check(same, "the per-lookup frame gives the per-tap reference's offsets");
  }

} // namespace

int main()
{
  testDisc();
  testTapOffsets();
  testReceiverPlane();
  testFactoredFrame();
  testRadiusAndTexelScaling();
  testNoise();
  testValidity();
  testShaderMatchesMirror();
  testEarlyTaps();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
