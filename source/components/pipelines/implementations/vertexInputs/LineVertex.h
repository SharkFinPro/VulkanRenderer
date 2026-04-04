#ifndef VKE_LINEVERTEX_H
#define VKE_LINEVERTEX_H

#include <vulkan/vulkan_raii.hpp>
#include <glm/vec3.hpp>
#include <array>

namespace vke {

  struct LineVertex {
    glm::vec3 pos;

    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(LineVertex),
        .inputRate = vk::VertexInputRate::eVertex
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(LineVertex, pos)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_LINEVERTEX_H
