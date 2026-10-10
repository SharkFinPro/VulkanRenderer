#ifndef VKE_DEBUGSHAPES_H
#define VKE_DEBUGSHAPES_H

#include <glm/glm.hpp>
#include <vector>

namespace vke {

  struct DebugSegment {
    glm::vec3 start;
    glm::vec3 end;
  };

  // Wireframe shapes as line segments, appended to `out`. Degenerate input (zero length or radius, non-finite
  // values) yields fewer segments or none, never NaNs. Every segment counts against the line limit.
  namespace debugShapes {

    constexpr int s_circleSegments = 32;
    constexpr int s_hemisphereSegments = 16;
    constexpr int s_arrowHeadSegments = 8;

    // 12 segments
    void appendBox(std::vector<DebugSegment>& out, const glm::mat4& transform, glm::vec3 halfExtents);

    // 3 great circles: 96 segments
    void appendSphere(std::vector<DebugSegment>& out, glm::vec3 center, float radius);

    // 2 rings, 4 side lines and 2 hemispheres of 2 arcs: 132 segments. a == b gives a sphere.
    void appendCapsule(std::vector<DebugSegment>& out, glm::vec3 a, glm::vec3 b, float radius);

    // 2 rings and 4 side lines: 68 segments
    void appendCylinder(std::vector<DebugSegment>& out, glm::vec3 a, glm::vec3 b, float radius);

    // A base ring and 4 lines to the apex: 36 segments
    void appendCone(std::vector<DebugSegment>& out, glm::vec3 apex, glm::vec3 baseCenter, float radius);

    // A shaft plus a head 20% of the length long: 13 segments
    void appendArrow(std::vector<DebugSegment>& out, glm::vec3 from, glm::vec3 to);

    // The 12 edges of the volume viewProjection maps to clip space, with Vulkan's 0..1 depth. Nothing for a
    // matrix that cannot be inverted.
    void appendFrustum(std::vector<DebugSegment>& out, const glm::mat4& viewProjection);

  } // namespace debugShapes

} // namespace vke

#endif //VKE_DEBUGSHAPES_H
