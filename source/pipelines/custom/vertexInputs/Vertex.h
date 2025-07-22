#ifndef VULKANPROJECT_VERTEX_H
#define VULKANPROJECT_VERTEX_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <array>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static constexpr VkVertexInputBindingDescription getBindingDescription()
  {
    return {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
  {
    return {{
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, pos)
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)
      },
      {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, texCoord)
      }
    }};
  }
};


#endif //VULKANPROJECT_VERTEX_H
