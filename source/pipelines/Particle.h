#ifndef PARTICLE_H
#define PARTICLE_H

#include <vulkan/vulkan.h>
#include <array>
#include <glm/glm.hpp>


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


#endif //PARTICLE_H
