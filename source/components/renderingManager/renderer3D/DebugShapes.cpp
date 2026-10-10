#include "DebugShapes.h"
#include <array>
#include <cmath>
#include <numbers>

namespace vke::debugShapes {

  namespace {
    struct Basis {
      glm::vec3 axis;
      glm::vec3 u;
      glm::vec3 v;
      float length;
    };

    bool isFinite(const glm::vec3 value)
    {
      return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    // An orthonormal basis around the direction from a to b (up when they coincide), built from the world axis
    // least aligned with it.
    Basis makeBasis(const glm::vec3 a, const glm::vec3 b)
    {
      const glm::vec3 delta = b - a;
      const float length = glm::length(delta);

      Basis basis { { 0.0f, 1.0f, 0.0f }, {}, {}, 0.0f };
      if (length > 1.0e-6f)
      {
        basis.axis = delta / length;
        basis.length = length;
      }

      const glm::vec3 absAxis = glm::abs(basis.axis);
      glm::vec3 helper { 1.0f, 0.0f, 0.0f };
      if (absAxis.y <= absAxis.x && absAxis.y <= absAxis.z)
      {
        helper = { 0.0f, 1.0f, 0.0f };
      }
      else if (absAxis.z <= absAxis.x && absAxis.z <= absAxis.y)
      {
        helper = { 0.0f, 0.0f, 1.0f };
      }

      basis.u = glm::normalize(glm::cross(basis.axis, helper));
      basis.v = glm::cross(basis.axis, basis.u);
      return basis;
    }

    // An arc from angle `from` to `to` in the plane spanned by u and v
    void appendArc(std::vector<DebugSegment>& out,
                   const glm::vec3 center,
                   const glm::vec3 u,
                   const glm::vec3 v,
                   const float radius,
                   const float from,
                   const float to,
                   const int segments)
    {
      auto pointAt = [&](const int i) {
        const float angle = from + (to - from) * static_cast<float>(i) / static_cast<float>(segments);
        return center + radius * (std::cos(angle) * u + std::sin(angle) * v);
      };

      glm::vec3 previous = pointAt(0);
      for (int i = 1; i <= segments; ++i)
      {
        const glm::vec3 current = pointAt(i);
        out.push_back({ previous, current });
        previous = current;
      }
    }

    void appendCircle(std::vector<DebugSegment>& out,
                      const glm::vec3 center,
                      const glm::vec3 u,
                      const glm::vec3 v,
                      const float radius,
                      const int segments)
    {
      appendArc(out, center, u, v, radius, 0.0f, 2.0f * std::numbers::pi_v<float>, segments);
    }

    void appendSideLines(std::vector<DebugSegment>& out,
                         const glm::vec3 a,
                         const glm::vec3 b,
                         const Basis& basis,
                         const float radius)
    {
      for (const glm::vec3 offset : { basis.u, basis.v, -basis.u, -basis.v })
      {
        out.push_back({ a + radius * offset, b + radius * offset });
      }
    }

    // Edges join corners whose indices differ in exactly one bit
    void appendCubeEdges(std::vector<DebugSegment>& out, const std::array<glm::vec3, 8>& corners)
    {
      for (int i = 0; i < 8; ++i)
      {
        for (int bit = 1; bit < 8; bit <<= 1)
        {
          if (!(i & bit))
          {
            out.push_back({ corners[i], corners[i | bit] });
          }
        }
      }
    }
  } // namespace

  void appendBox(std::vector<DebugSegment>& out, const glm::mat4& transform, const glm::vec3 halfExtents)
  {
    std::array<glm::vec3, 8> corners;
    for (int i = 0; i < 8; ++i)
    {
      const glm::vec3 sign {
        (i & 1) ? 1.0f : -1.0f,
        (i & 2) ? 1.0f : -1.0f,
        (i & 4) ? 1.0f : -1.0f
      };
      corners[i] = glm::vec3(transform * glm::vec4(sign * halfExtents, 1.0f));
      if (!isFinite(corners[i]))
      {
        return;
      }
    }

    appendCubeEdges(out, corners);
  }

  void appendSphere(std::vector<DebugSegment>& out, const glm::vec3 center, const float radius)
  {
    if (!isFinite(center) || !std::isfinite(radius))
    {
      return;
    }

    const float r = std::abs(radius);
    appendCircle(out, center, { 1, 0, 0 }, { 0, 1, 0 }, r, s_circleSegments);
    appendCircle(out, center, { 0, 1, 0 }, { 0, 0, 1 }, r, s_circleSegments);
    appendCircle(out, center, { 0, 0, 1 }, { 1, 0, 0 }, r, s_circleSegments);
  }

  void appendCapsule(std::vector<DebugSegment>& out, const glm::vec3 a, const glm::vec3 b, const float radius)
  {
    if (!isFinite(a) || !isFinite(b) || !std::isfinite(radius))
    {
      return;
    }

    const float r = std::abs(radius);
    const Basis basis = makeBasis(a, b);

    appendCircle(out, a, basis.u, basis.v, r, s_circleSegments);
    appendCircle(out, b, basis.u, basis.v, r, s_circleSegments);
    appendSideLines(out, a, b, basis, r);

    // Half circles from one side of the ring over the pole to the other
    for (const glm::vec3 side : { basis.u, basis.v })
    {
      appendArc(out, b, side, basis.axis, r, 0.0f, std::numbers::pi_v<float>, s_hemisphereSegments);
      appendArc(out, a, side, -basis.axis, r, 0.0f, std::numbers::pi_v<float>, s_hemisphereSegments);
    }
  }

  void appendCylinder(std::vector<DebugSegment>& out, const glm::vec3 a, const glm::vec3 b, const float radius)
  {
    if (!isFinite(a) || !isFinite(b) || !std::isfinite(radius))
    {
      return;
    }

    const float r = std::abs(radius);
    const Basis basis = makeBasis(a, b);

    appendCircle(out, a, basis.u, basis.v, r, s_circleSegments);
    appendCircle(out, b, basis.u, basis.v, r, s_circleSegments);
    appendSideLines(out, a, b, basis, r);
  }

  void appendCone(std::vector<DebugSegment>& out, const glm::vec3 apex, const glm::vec3 baseCenter, const float radius)
  {
    if (!isFinite(apex) || !isFinite(baseCenter) || !std::isfinite(radius))
    {
      return;
    }

    const float r = std::abs(radius);
    const Basis basis = makeBasis(baseCenter, apex);

    appendCircle(out, baseCenter, basis.u, basis.v, r, s_circleSegments);
    for (const glm::vec3 offset : { basis.u, basis.v, -basis.u, -basis.v })
    {
      out.push_back({ baseCenter + r * offset, apex });
    }
  }

  void appendArrow(std::vector<DebugSegment>& out, const glm::vec3 from, const glm::vec3 to)
  {
    if (!isFinite(from) || !isFinite(to))
    {
      return;
    }

    const Basis basis = makeBasis(from, to);
    if (basis.length <= 1.0e-6f)
    {
      return;
    }

    const float headLength = 0.2f * basis.length;
    const float headRadius = 0.35f * headLength;
    const glm::vec3 headBase = to - basis.axis * headLength;

    out.push_back({ from, to });
    appendCircle(out, headBase, basis.u, basis.v, headRadius, s_arrowHeadSegments);
    for (const glm::vec3 offset : { basis.u, basis.v, -basis.u, -basis.v })
    {
      out.push_back({ headBase + headRadius * offset, to });
    }
  }

  void appendFrustum(std::vector<DebugSegment>& out, const glm::mat4& viewProjection)
  {
    const glm::mat4 inverse = glm::inverse(viewProjection);

    std::array<glm::vec3, 8> corners;
    for (int i = 0; i < 8; ++i)
    {
      // Any Y flip in the matrix is undone by the inverse, so the clip space corners need no special case
      const glm::vec4 clip {
        (i & 1) ? 1.0f : -1.0f,
        (i & 2) ? 1.0f : -1.0f,
        (i & 4) ? 1.0f : 0.0f,
        1.0f
      };
      const glm::vec4 world = inverse * clip;
      if (!std::isfinite(world.w) || std::abs(world.w) < 1.0e-12f)
      {
        return;
      }

      corners[i] = glm::vec3(world) / world.w;
      if (!isFinite(corners[i]))
      {
        return;
      }
    }

    appendCubeEdges(out, corners);
  }

} // namespace vke::debugShapes
