#ifndef VKE_LIGHTRANGE_H
#define VKE_LIGHTRANGE_H

#include <algorithm>
#include <cmath>

namespace vke {

  // True when range is 0 (no range) or finite and greater than the shadow near plane
  [[nodiscard]] inline bool isValidLightRange(const float range, const float shadowNearPlane)
  {
    return std::isfinite(range) && (range == 0.0f || range > shadowNearPlane);
  }

  // Shadows beyond where a light reaches would never be seen
  [[nodiscard]] inline float effectiveShadowRange(const float shadowRange, const float range)
  {
    return range > 0.0f ? std::min(shadowRange, range) : shadowRange;
  }

  // Must match lightAttenuation in shaders/common/Lighting.glsl.
  // 1 without a range; otherwise inverse square (+1, so 1 at the light) windowed smoothly to 0 at the range.
  [[nodiscard]] inline float lightAttenuation(const float range, const float distance)
  {
    if (range <= 0.0f)
    {
      return 1.0f;
    }

    if (distance >= range)
    {
      return 0.0f;
    }

    const float ratio = (distance * distance) / (range * range);
    const float window = std::clamp(1.0f - ratio * ratio, 0.0f, 1.0f);

    return window * window / (distance * distance + 1.0f);
  }

} // namespace vke

#endif //VKE_LIGHTRANGE_H
