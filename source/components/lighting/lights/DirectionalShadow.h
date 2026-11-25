#ifndef VKE_DIRECTIONALSHADOW_H
#define VKE_DIRECTIONALSHADOW_H

#include "../../../utilities/Bounds.h"

#include <glm/ext/matrix_clip_space.hpp>
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
#include <initializer_list>

namespace vke {

  // The most cascades a directional light's shadow map holds; it always has this many layers
  inline constexpr uint32_t s_maxShadowCascades = 4;

  // What the camera contributes to fitting the cascades
  struct CascadeCamera {
    glm::mat4 view; // World to camera view; the camera looks down -Z
    float fieldOfViewDegrees; // Vertical
    float aspect; // Width over height
    float nearPlane;
  };

  // The view depths that bound the cascades' slices of the camera frustum: result[0] is nearPlane, result[i] the far edge
  // of cascade i - 1, result[count] is shadowDistance. Each split blends the logarithmic and the uniform distribution
  // (the practical split): lambda 1 is logarithmic, 0 is uniform. Entries past count equal shadowDistance.
  [[nodiscard]] inline std::array<float, s_maxShadowCascades + 1> cascadeSplitDistances(const float nearPlane,
                                                                                         const float shadowDistance,
                                                                                         const uint32_t count,
                                                                                         const float lambda)
  {
    std::array<float, s_maxShadowCascades + 1> splits;
    splits.fill(shadowDistance);
    splits[0] = nearPlane;

    for (uint32_t i = 1; i < count; ++i)
    {
      const float fraction = static_cast<float>(i) / static_cast<float>(count);
      const float logarithmic = nearPlane * std::pow(shadowDistance / nearPlane, fraction);
      const float uniform = nearPlane + (shadowDistance - nearPlane) * fraction;

      splits[i] = lambda * logarithmic + (1.0f - lambda) * uniform;
    }

    return splits;
  }

  // The fixed view of the light: at the origin, looking along direction (the way the light travels). It does not follow
  // the camera, so the shadow map's texels stay put in the world while the camera moves.
  [[nodiscard]] inline glm::mat4 directionalLightView(const glm::vec3 direction)
  {
    const glm::vec3 dir = glm::normalize(direction);

    // As SpotLight does, to keep the view basis defined when looking straight up or down
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

    return glm::lookAt(glm::vec3(0.0f), dir, up);
  }

  struct ShadowCascade {
    glm::mat4 viewProjection; // Depth [0, 1], y flipped for Vulkan
    float depthBias; // In normalized depth
  };

  // Fits one cascade: an orthographic projection along direction over the part of the camera frustum between the view
  // depths sliceNear and sliceFar. It covers the bounding sphere of that slice, so its size depends only on the slice's
  // shape and not on where the camera looks, and its center is snapped to whole texels, so the map doesn't shimmer as the
  // camera moves (the box is a texel larger than the sphere for that). The depth range also reaches back to casterBounds
  // (the world box of every shadow caster; may be empty), so casters between the light and the slice, outside the slice
  // itself, still shadow it. The bias is 2 texels along the light.
  [[nodiscard]] inline ShadowCascade fitShadowCascade(const glm::vec3 direction,
                                                      const CascadeCamera& camera,
                                                      const float sliceNear,
                                                      const float sliceFar,
                                                      const uint32_t mapSize,
                                                      const AABB& casterBounds)
  {
    const glm::mat4 inverseView = glm::inverse(camera.view);
    const float tanHalfFov = std::tan(glm::radians(camera.fieldOfViewDegrees) * 0.5f);

    std::array<glm::vec3, 8> corners;
    size_t cornerCount = 0;

    for (const float depth : { sliceNear, sliceFar })
    {
      const float halfHeight = depth * tanHalfFov;
      const float halfWidth = halfHeight * camera.aspect;

      for (const float y : { -halfHeight, halfHeight })
      {
        for (const float x : { -halfWidth, halfWidth })
        {
          corners[cornerCount++] = glm::vec3(inverseView * glm::vec4(x, y, -depth, 1.0f));
        }
      }
    }

    glm::vec3 center(0.0f);

    for (const glm::vec3& corner : corners)
    {
      center += corner;
    }

    center /= 8.0f;

    float radius = 0.0f;

    for (const glm::vec3& corner : corners)
    {
      radius = std::max(radius, glm::length(corner - center));
    }

    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Snapping the center moves the box by less than a texel (2 * radius / mapSize); grow it by that much so the sphere
    // stays inside
    radius *= static_cast<float>(mapSize) / static_cast<float>(mapSize - 2);

    const glm::mat4 lightView = directionalLightView(direction);
    glm::vec3 lightCenter = glm::vec3(lightView * glm::vec4(center, 1.0f));

    const float texel = 2.0f * radius / static_cast<float>(mapSize);

    // Receivers lie within the sphere; distance along the light is -z
    float nearDistance = -lightCenter.z - radius;
    const float farDistance = -lightCenter.z + radius;

    if (!casterBounds.isEmpty())
    {
      nearDistance = std::min(nearDistance, -transformAABB(casterBounds, lightView).max.z);
    }

    lightCenter.x = std::floor(lightCenter.x / texel) * texel;
    lightCenter.y = std::floor(lightCenter.y / texel) * texel;

    glm::mat4 projection = glm::ortho(lightCenter.x - radius, lightCenter.x + radius,
                                      lightCenter.y - radius, lightCenter.y + radius, nearDistance, farDistance);

    // Vulkan's y points down; the off-center box needs its translation flipped too
    projection[1][1] *= -1;
    projection[3][1] *= -1;

    return { projection * lightView, 2.0f * texel / (farDistance - nearDistance) };
  }

} // namespace vke

#endif //VKE_DIRECTIONALSHADOW_H
