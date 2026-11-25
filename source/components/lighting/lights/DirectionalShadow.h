#ifndef VKE_DIRECTIONALSHADOW_H
#define VKE_DIRECTIONALSHADOW_H

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <cmath>
#include <cstdint>

namespace vke {

  // The view-projection of a directional light's single shadow map: an orthographic projection looking along direction
  // (the way the light travels) at a cube of side 2 * halfExtent centered on center. Depth is [0, 1], 0 on the face of the
  // cube the light enters and 1 on the face it leaves, so the center lands at NDC (0, 0) and depth 0.5. Only shadow casters
  // inside the cube are in the map; cascaded shadow maps will replace this fitting, so keep it the only place that decides
  // how the map covers the scene.
  [[nodiscard]] inline glm::mat4 directionalShadowViewProjection(const glm::vec3 direction,
                                                                 const glm::vec3 center,
                                                                 const float halfExtent)
  {
    const glm::vec3 dir = glm::normalize(direction);

    // As SpotLight does, to keep the view basis defined when looking straight up or down
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

    const glm::mat4 view = glm::lookAt(center - dir * halfExtent, center, up);

    glm::mat4 proj = glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, 0.0f, 2.0f * halfExtent);

    proj[1][1] *= -1;

    return proj * view;
  }

  // The depth bias, in normalized depth, of 2 texels of the shadow map along the light. A texel is 2 * halfExtent / mapSize
  // in world units and the depth range spans 2 * halfExtent, so the bias is 2 / mapSize whatever the extent.
  [[nodiscard]] inline float directionalShadowBias(const uint32_t mapSize)
  {
    return 2.0f / static_cast<float>(mapSize);
  }

} // namespace vke

#endif //VKE_DIRECTIONALSHADOW_H
