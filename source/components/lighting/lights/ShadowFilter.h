#ifndef VKE_SHADOWFILTER_H
#define VKE_SHADOWFILTER_H

#include "ShadowBias.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace vke {

  // Percentage-closer filtering of a shadow lookup: s_shadowFilterTaps comparison samples spread over a disc around the
  // lookup position, in the plane facing the light, and averaged. The radius is in texels of the light's shadow map (the
  // world size of one texel at the fragment), like ShadowBias; 0 takes a single sample.
  inline constexpr float s_defaultShadowFilterRadius = 1.0f;
  inline constexpr int s_shadowFilterTaps = 16;

  // Must match shadowFilterDisc in shaders/common/SceneLighting.glsl.
  // Well spread points inside the unit disc, centered on its origin
  inline const std::array<glm::vec2, s_shadowFilterTaps> s_shadowFilterDisc = { {
    { 0.032389f, 0.062461f },
    { -0.678361f, -0.673981f },
    { 0.571391f, -0.800312f },
    { -0.482854f, 0.854702f },
    { 0.848794f, 0.156892f },
    { -0.938362f, 0.220159f },
    { 0.530101f, 0.783952f },
    { 0.029889f, -0.650125f },
    { -0.425273f, -0.058974f },
    { -0.046156f, 0.544012f },
    { 0.855842f, -0.370545f },
    { 0.386474f, 0.335194f },
    { 0.054615f, 0.958519f },
    { 0.410468f, -0.288637f },
    { -0.934812f, -0.243051f },
    { -0.321681f, -0.880796f }
  } };

  // The widest filter a light takes, in texels; far past any useful softness, and it keeps radius * texel finite
  inline constexpr float s_maxShadowFilterRadius = 64.0f;

  // True when the radius is in [0, s_maxShadowFilterRadius]
  [[nodiscard]] inline bool isValidShadowFilterRadius(const float radius)
  {
    return std::isfinite(radius) && radius >= 0.0f && radius <= s_maxShadowFilterRadius;
  }

  // Must match interleavedGradientNoise in shaders/common/SceneLighting.glsl.
  // A per-pixel value in [0, 1) that rotates the disc, so the banding of a fixed pattern turns into fine noise
  [[nodiscard]] inline float interleavedGradientNoise(const glm::vec2& fragCoord)
  {
    const auto fract = [](const float x) { return x - std::floor(x); };

    return fract(52.9829189f * fract(glm::dot(fragCoord, glm::vec2(0.06711056f, 0.00583715f))));
  }

  // Must match shadowFilterOffset in shaders/common/SceneLighting.glsl.
  // The world space offset of one tap: the disc point, rotated by rotation (radians), scaled by radius (texels) and texel
  // (world size of a texel), laid on the plane perpendicular to toLight (a unit vector)
  [[nodiscard]] inline glm::vec3 shadowFilterTapOffset(const int tap,
                                                       const float rotation,
                                                       const float radius,
                                                       const float texel,
                                                       const glm::vec3& toLight)
  {
    const glm::vec3 absLight = glm::abs(toLight);
    const glm::vec3 helper = absLight.x <= absLight.y && absLight.x <= absLight.z ? glm::vec3(1.0f, 0.0f, 0.0f)
                             : absLight.y <= absLight.z ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                        : glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 t = glm::normalize(glm::cross(toLight, helper));
    const glm::vec3 b = glm::cross(toLight, t);

    const glm::vec2 d = s_shadowFilterDisc[static_cast<size_t>(tap)];
    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    const glm::vec2 rotated(c * d.x - s * d.y, s * d.x + c * d.y);

    return (t * rotated.x + b * rotated.y) * (radius * texel);
  }

  // A tap offset (in the plane facing the light) slid along the unit toLight onto the receiver's plane, the plane through
  // the fragment with the unit normal, as the GLSL shadowFilterOffset does: a tap left in the disc's plane would sit below
  // a surface the light grazes and read as shadowed. The slide is clamped to 10 times the offset's length, like the slope
  // bias's tangent.
  [[nodiscard]] inline glm::vec3 receiverPlaneTapOffset(const glm::vec3& offset,
                                                        const glm::vec3& normal,
                                                        const glm::vec3& toLight)
  {
    const float limit = s_maxShadowBiasTangent * glm::length(offset);
    const float slide = std::clamp(-glm::dot(offset, normal) / std::max(glm::dot(normal, toLight), 1e-4f), -limit, limit);

    return offset + toLight * slide;
  }

  // Must match ShadowFilterFrame, shadowFilterFrame and shadowFilterOffset in shaders/common/SceneLighting.glsl.
  // The shader's factored form of receiverPlaneTapOffset(shadowFilterTapOffset(...)): the offset and its slide are linear
  // in the disc point, so everything else is folded once per lookup and a tap costs two multiply-adds and a clamp.
  struct ShadowFilterFrame
  {
    glm::vec3 t;
    glm::vec3 b;
    glm::vec3 toLight;
    glm::vec2 slide;
    float limit;
  };

  // t and b are an orthonormal basis of the plane perpendicular to the unit toLight; rotation is (cos, sin)
  [[nodiscard]] inline ShadowFilterFrame shadowFilterFrame(const glm::vec2& rotation,
                                                           const glm::vec3& t,
                                                           const glm::vec3& b,
                                                           const float scale,
                                                           const glm::vec3& normal,
                                                           const glm::vec3& toLight)
  {
    ShadowFilterFrame frame;
    frame.t = (t * rotation.x + b * rotation.y) * scale;
    frame.b = (b * rotation.x - t * rotation.y) * scale;
    frame.toLight = toLight;
    frame.slide = -glm::vec2(glm::dot(frame.t, normal), glm::dot(frame.b, normal)) /
                  std::max(glm::dot(normal, toLight), 1e-4f);
    frame.limit = s_maxShadowBiasTangent * scale;

    return frame;
  }

  [[nodiscard]] inline glm::vec3 shadowFilterFrameOffset(const int tap,
                                                         const ShadowFilterFrame& frame)
  {
    const glm::vec2 d = s_shadowFilterDisc[static_cast<size_t>(tap)];
    const float limit = frame.limit * glm::length(d);
    const float slide = std::clamp(glm::dot(d, frame.slide), -limit, limit);

    return frame.t * d.x + frame.b * d.y + frame.toLight * slide;
  }

} // namespace vke

#endif //VKE_SHADOWFILTER_H
