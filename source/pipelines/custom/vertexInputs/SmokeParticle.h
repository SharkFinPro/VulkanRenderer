#ifndef SMOKEPARTICLE_H
#define SMOKEPARTICLE_H

#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace vke {

struct SmokeParticle {
  glm::vec4 positionTtl;
  glm::vec4 velocityColor;

  static constexpr VkVertexInputBindingDescription getBindingDescription()
  {
    return {
      .binding = 0,
      .stride = sizeof(SmokeParticle),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
  {
    return {{
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(SmokeParticle, positionTtl)
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(SmokeParticle, velocityColor)
      }
    }};
  }
};

} // namespace vke

#endif //SMOKEPARTICLE_H
