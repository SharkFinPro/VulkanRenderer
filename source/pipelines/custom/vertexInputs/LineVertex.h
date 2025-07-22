#ifndef LINEVERTEX_H
#define LINEVERTEX_H

#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <array>

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

#endif //LINEVERTEX_H
