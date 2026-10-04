#ifndef VKE_PICKRESULT_H
#define VKE_PICKRESULT_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <memory>

namespace vke {

  class RenderObject;

  struct PickResult {
    std::shared_ptr<RenderObject> renderObject;

    // Triangle number within the object's draw (indices 3n to 3n + 2 of its index buffer, since a model is one draw).
    uint32_t triangleIndex;

    glm::vec3 worldPosition;

    // 0..1 depth the picking pass wrote at the pixel.
    float depth;
  };

} // namespace vke

#endif //VKE_PICKRESULT_H
