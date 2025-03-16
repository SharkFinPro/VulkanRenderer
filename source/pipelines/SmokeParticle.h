#ifndef SMOKEPARTICLE_H
#define SMOKEPARTICLE_H

#include <vulkan/vulkan.h>
#include <array>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct SmokeParticle {
  glm::vec3 position;
  float ttl;
  glm::vec3 velocity;
  float padding;
  glm::vec4 color;
  glm::vec3 initialPosition;
  float padding2;
  glm::vec3 initialVelocity;
  float padding3;

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
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(SmokeParticle, position)
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(SmokeParticle, color)
      }
    }};
  }
};

#endif //SMOKEPARTICLE_H
