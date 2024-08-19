#ifndef VULKANPROJECT_VERTEX_H
#define VULKANPROJECT_VERTEX_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    constexpr VkVertexInputBindingDescription bindingDescription {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
  {
    constexpr std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {{
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

    return attributeDescriptions;
  }
};


#endif //VULKANPROJECT_VERTEX_H
