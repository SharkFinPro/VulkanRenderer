#ifndef VKE_SMOKEPARTICLE_H
#define VKE_SMOKEPARTICLE_H

#include <glm/vec4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>

namespace vke {

  struct SmokeParticle {
    glm::vec4 positionTtl;
    glm::vec4 velocityColor;

    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(SmokeParticle),
        .inputRate = vk::VertexInputRate::eVertex
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32A32Sfloat,
          .offset = offsetof(SmokeParticle, positionTtl)
        },
        {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32A32Sfloat,
          .offset = offsetof(SmokeParticle, velocityColor)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_SMOKEPARTICLE_H
