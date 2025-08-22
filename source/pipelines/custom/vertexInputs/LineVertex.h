#ifndef VKE_LINEVERTEX_H
#define VKE_LINEVERTEX_H

#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <array>

namespace vke {

struct LineVertex {
  glm::vec3 pos;

  static constexpr VkVertexInputBindingDescription getBindingDescription()
  {
    return {
      .binding = 0,
      .stride = sizeof(LineVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
  {
    return {{
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(LineVertex, pos)
      }
    }};
  }
};

} // namespace vke

#endif //VKE_LINEVERTEX_H
