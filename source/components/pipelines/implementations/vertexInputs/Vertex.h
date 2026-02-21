#ifndef VKE_VERTEX_H
#define VKE_VERTEX_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace vke {

  struct Vertex {
    glm::vec3 pos;
    float padding1;
    glm::vec3 normal;
    float padding2;
    glm::vec2 texCoord;
    glm::vec2 padding3;

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

    static constexpr std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptionsPositionOnly()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = offsetof(Vertex, pos)
        }
      }};
    }

    static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptionsPositionAndNormal()
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
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_VERTEX_H
