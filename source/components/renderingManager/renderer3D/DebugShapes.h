#ifndef VKE_DEBUGSHAPES_H
#define VKE_DEBUGSHAPES_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace vke {

  struct DebugSegment {
    glm::vec3 start;
    glm::vec3 end;
  };

  // Wireframe shapes as line segments, appended to `out`. Non-finite input and a zero radius append nothing, and a
  // zero-length axis falls back to up, so no shape produces NaNs or zero-length segments. Every segment counts
  // against the line limit.
  namespace debugShapes {

    constexpr int s_circleSegments = 32;
    constexpr int s_hemisphereSegments = 16;
    constexpr int s_arrowHeadSegments = 8;

    // 12 segments
    void appendBox(std::vector<DebugSegment>& out, const glm::mat4& transform, glm::vec3 halfExtents);

    // 3 great circles: 96 segments
    void appendSphere(std::vector<DebugSegment>& out, glm::vec3 center, float radius);

    // 2 rings, 4 side lines and 2 hemispheres of 2 arcs: 132 segments. a == b gives just the sphere (96).
    void appendCapsule(std::vector<DebugSegment>& out, glm::vec3 a, glm::vec3 b, float radius);

    // 2 rings and 4 side lines: 68 segments. a == b gives one ring (32).
    void appendCylinder(std::vector<DebugSegment>& out, glm::vec3 a, glm::vec3 b, float radius);

    // A base ring and 4 lines to the apex: 36 segments
    void appendCone(std::vector<DebugSegment>& out, glm::vec3 apex, glm::vec3 baseCenter, float radius);

    // A shaft plus a head 20% of the length long: 13 segments. Nothing for a zero-length arrow.
    void appendArrow(std::vector<DebugSegment>& out, glm::vec3 from, glm::vec3 to);

    // The 12 edges of the volume viewProjection maps to clip space, with Vulkan's 0..1 depth. Nothing when a
    // corner comes out non-finite or at infinity, as for a singular matrix or an infinite far plane
    // (glm::infinitePerspective).
    void appendFrustum(std::vector<DebugSegment>& out, const glm::mat4& viewProjection);

  } // namespace debugShapes

} // namespace vke

#endif //VKE_DEBUGSHAPES_H
