#ifndef VKE_FRUSTUM_H
#define VKE_FRUSTUM_H

#include "Bounds.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <array>

namespace vke {

  // Six normalized planes (xyz = normal pointing inside, w = distance) of a view frustum.
  // The tests are conservative: they never reject something visible, but may accept a box or sphere that is
  // just outside near a frustum corner or edge.
  class Frustum {
  public:
    // Extracts the planes from a view-projection matrix with a [0, 1] depth range (Vulkan, or GLM with
    // GLM_FORCE_DEPTH_ZERO_TO_ONE). A Y-flipped projection works too; it only swaps the top and bottom planes.
    [[nodiscard]] static Frustum fromViewProjection(const glm::mat4& viewProjection);

    // False for an empty box, or one entirely behind any plane.
    [[nodiscard]] bool intersects(const AABB& box) const;

    [[nodiscard]] bool intersects(glm::vec3 center,
                                  float radius) const;

    [[nodiscard]] const std::array<glm::vec4, 6>& getPlanes() const;

  private:
    std::array<glm::vec4, 6> m_planes {};
  };

} // namespace vke

#endif //VKE_FRUSTUM_H
