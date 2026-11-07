#ifndef VKE_BOUNDS_H
#define VKE_BOUNDS_H

#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <cmath>
#include <limits>

namespace vke {

  // Axis-aligned bounding box. A default box is empty (min above max) and grows with expand().
  struct AABB {
    glm::vec3 min { std::numeric_limits<float>::infinity() };
    glm::vec3 max { -std::numeric_limits<float>::infinity() };

    void expand(const glm::vec3 point)
    {
      min = glm::min(min, point);
      max = glm::max(max, point);
    }

    [[nodiscard]] bool isEmpty() const
    {
      return min.x > max.x || min.y > max.y || min.z > max.z;
    }

    [[nodiscard]] glm::vec3 center() const
    {
      return (min + max) * 0.5f;
    }

    [[nodiscard]] glm::vec3 halfExtents() const
    {
      return (max - min) * 0.5f;
    }
  };

  // The world-space box enclosing a box under an affine transform (Arvo's method). The transform must be
  // affine (no perspective row). An empty box stays empty.
  [[nodiscard]] inline AABB transformAABB(const AABB& box,
                                          const glm::mat4& transform)
  {
    if (box.isEmpty())
    {
      return {};
    }

    const glm::vec3 center = box.center();
    const glm::vec3 halfExtents = box.halfExtents();

    // glm is column-major: transform[column][row]. New half extent i = sum over j of |M(i, j)| * halfExtent j.
    const glm::vec3 newCenter = glm::vec3(transform * glm::vec4(center, 1.0f));
    glm::vec3 newHalfExtents { 0.0f };

    for (int row = 0; row < 3; ++row)
    {
      for (int column = 0; column < 3; ++column)
      {
        newHalfExtents[row] += std::abs(transform[column][row]) * halfExtents[column];
      }
    }

    return { newCenter - newHalfExtents, newCenter + newHalfExtents };
  }

} // namespace vke

#endif //VKE_BOUNDS_H
