#ifndef VKE_SHADOWBIAS_H
#define VKE_SHADOWBIAS_H

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <cmath>

namespace vke {

  // How a shadow lookup is moved off the surface so it doesn't shadow itself. All three are in texels of the light's shadow
  // map (the world size of one texel at the fragment), so they hold up across distance, resolution and light type.
  struct ShadowBias {
    float constant = 1.0f; // Toward the light, always
    float slope = 1.0f; // Toward the light, times the tangent of the angle between the normal and the light
    float normalOffset = 0.0f; // Along the surface normal; off by default, since it lifts lookups past the base of a caster resting on the surface
  };

  // The slope term stops growing at this tangent, so surfaces seen edge on don't push the lookup far off
  inline constexpr float s_maxShadowBiasTangent = 10.0f;

  // True when all three are finite and not negative
  [[nodiscard]] inline bool isValidShadowBias(const ShadowBias& bias)
  {
    return std::isfinite(bias.constant) && bias.constant >= 0.0f &&
           std::isfinite(bias.slope) && bias.slope >= 0.0f &&
           std::isfinite(bias.normalOffset) && bias.normalOffset >= 0.0f;
  }

  // World size of one texel of a point light's cube face per unit of view depth (90 degree faces)
  [[nodiscard]] inline float pointShadowTexelPerDepth(const float mapSize)
  {
    return 2.0f / mapSize;
  }

  // The same for a spot light's map, fovDegrees being the full field of view of its projection
  [[nodiscard]] inline float spotShadowTexelPerDepth(const float fovDegrees,
                                                     const float mapSize)
  {
    return 2.0f * std::tan(glm::radians(fovDegrees) * 0.5f) / mapSize;
  }

  // Must match shadowLookupPosition in shaders/common/Lighting.glsl.
  // The position the shadow map is looked up at: the fragment moved along its normal and toward the light. normal and
  // toLight are unit vectors, texel the world size of a shadow map texel at the fragment, bias the three terms
  // (constant, slope, normal offset) in texels.
  [[nodiscard]] inline glm::vec3 shadowLookupPosition(const glm::vec3& fragPos,
                                                      const glm::vec3& normal,
                                                      const glm::vec3& toLight,
                                                      const float texel,
                                                      const glm::vec3& bias)
  {
    const float cosTheta = std::clamp(glm::dot(normal, toLight), 0.0f, 1.0f);
    const float tanTheta = std::min(std::sqrt(1.0f - cosTheta * cosTheta) / std::max(cosTheta, 1e-4f),
                                    s_maxShadowBiasTangent);

    return fragPos + normal * (bias.z * texel) + toLight * (texel * (bias.x + bias.y * tanTheta));
  }

} // namespace vke

#endif //VKE_SHADOWBIAS_H
