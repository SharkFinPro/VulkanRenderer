#ifndef VKE_LINEINSTANCE_H
#define VKE_LINEINSTANCE_H

#include <vulkan/vulkan_raii.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <array>

namespace vke {

  // One line segment; the vertex shader expands it into a screen-space quad.
  struct LineInstance {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec4 color;
    // Framebuffer pixels of the scene image
    float width;

    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(LineInstance),
        .inputRate = vk::VertexInputRate::eInstance
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(LineInstance, start)
        },
        {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(LineInstance, end)
        },
        {
          .location = 2,
          .binding = 0,
          .format = vk::Format::eR32G32B32A32Sfloat,
          .offset = offsetof(LineInstance, color)
        },
        {
          .location = 3,
          .binding = 0,
          .format = vk::Format::eR32Sfloat,
          .offset = offsetof(LineInstance, width)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_LINEINSTANCE_H
