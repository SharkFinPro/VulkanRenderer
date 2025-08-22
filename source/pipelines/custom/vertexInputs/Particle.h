#ifndef VKE_PARTICLE_H
#define VKE_PARTICLE_H

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace vke {

struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;

  static constexpr VkVertexInputBindingDescription getBindingDescription()
  {
    return {
      .binding = 0,
      .stride = sizeof(Particle),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
  {
    return {{
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Particle, position)
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Particle, color)
      }
    }};
  }
};

} // namespace vke

#endif //VKE_PARTICLE_H
