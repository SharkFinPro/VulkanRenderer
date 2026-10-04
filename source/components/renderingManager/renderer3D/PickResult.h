#ifndef VKE_PICKRESULT_H
#define VKE_PICKRESULT_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>

namespace vke {

  class RenderObject;

  struct PickResult {
    std::shared_ptr<RenderObject> renderObject;

    // Which source mesh of the model was hit (Model draws all its meshes with one indexed draw).
    uint32_t meshIndex;

    // Within the model's index buffer.
    uint32_t triangleIndex;

    glm::vec3 worldPosition;

    // 0..1 depth the picking pass wrote at the pixel.
    float depth;
  };

} // namespace vke

#endif //VKE_PICKRESULT_H
