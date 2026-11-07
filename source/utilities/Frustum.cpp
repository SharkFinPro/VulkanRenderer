#include "Frustum.h"
#include <glm/geometric.hpp>

namespace vke {

  Frustum Frustum::fromViewProjection(const glm::mat4& viewProjection)
  {
    // Row i of the matrix; glm stores columns.
    const auto row = [&viewProjection](const int i) {
      return glm::vec4(viewProjection[0][i], viewProjection[1][i], viewProjection[2][i], viewProjection[3][i]);
    };

    const glm::vec4 row0 = row(0);
    const glm::vec4 row1 = row(1);
    const glm::vec4 row2 = row(2);
    const glm::vec4 row3 = row(3);

    Frustum frustum;
    frustum.m_planes = {
      row3 + row0, // left
      row3 - row0, // right
      row3 + row1, // bottom
      row3 - row1, // top
      row2,        // near: depth 0 at the near plane, unlike OpenGL's row3 + row2
      row3 - row2  // far
    };

    for (glm::vec4& plane : frustum.m_planes)
    {
      plane /= glm::length(glm::vec3(plane));
    }

    return frustum;
  }

  bool Frustum::intersects(const AABB& box) const
  {
    if (box.isEmpty())
    {
      return false;
    }

    for (const glm::vec4& plane : m_planes)
    {
      // The corner furthest along the normal; if even that is behind the plane, the whole box is.
      const glm::vec3 positive {
        plane.x >= 0.0f ? box.max.x : box.min.x,
        plane.y >= 0.0f ? box.max.y : box.min.y,
        plane.z >= 0.0f ? box.max.z : box.min.z
      };

      if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f)
      {
        return false;
      }
    }

    return true;
  }

  bool Frustum::intersects(const glm::vec3 center,
                           const float radius) const
  {
    for (const glm::vec4& plane : m_planes)
    {
      if (glm::dot(glm::vec3(plane), center) + plane.w < -radius)
      {
        return false;
      }
    }

    return true;
  }

  const std::array<glm::vec4, 6>& Frustum::getPlanes() const
  {
    return m_planes;
  }

} // namespace vke
