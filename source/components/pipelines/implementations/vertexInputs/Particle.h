#ifndef VKE_PARTICLE_H
#define VKE_PARTICLE_H

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>

namespace vke {

  struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(Particle),
        .inputRate = vk::VertexInputRate::eVertex
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32Sfloat,
          .offset = offsetof(Particle, position)
        },
        {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32A32Sfloat,
          .offset = offsetof(Particle, color)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_PARTICLE_H
