#ifndef VKE_PICKRESULT_H
#define VKE_PICKRESULT_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>

namespace vke {

  class RenderObject;
  class VolumeSurface;

  struct PickResult {
    // Exactly one of renderObject and volumeSurface is set.
    std::shared_ptr<RenderObject> renderObject;

    std::shared_ptr<VolumeSurface> volumeSurface;

    // Triangle number within the object's draw (indices 3n to 3n + 2 of its index buffer, since a model is one draw).
    // For a volume surface it is the triangle within one chunk's mesh, which is internal and has no stable meaning.
    uint32_t triangleIndex;

    glm::vec3 worldPosition;

    // 0..1 depth the picking pass wrote at the pixel.
    float depth;
  };

} // namespace vke

#endif //VKE_PICKRESULT_H
