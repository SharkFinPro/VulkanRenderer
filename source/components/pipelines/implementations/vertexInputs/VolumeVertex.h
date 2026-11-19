#ifndef VKE_VOLUMEVERTEX_H
#define VKE_VOLUMEVERTEX_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace vke {

  // The vertex a volume surface's meshes are drawn with. The materials and weights are four-way blend inputs, of which
  // only the first material (weight 255) is used until surfaces blend materials.
  struct VolumeVertex {
    glm::vec3 position;
    glm::vec3 normal;
    std::array<uint8_t, 4> materials;
    std::array<uint8_t, 4> weights;
  };

  static_assert(sizeof(VolumeVertex) == 32);

  struct VolumeVertexInput {
    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(VolumeVertex),
        .inputRate = vk::VertexInputRate::eVertex
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(VolumeVertex, position)
        },
        {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(VolumeVertex, normal)
        },
        {
          .location = 2,
          .binding = 0,
          .format = vk::Format::eR8G8B8A8Uint,
          .offset = offsetof(VolumeVertex, materials)
        },
        {
          .location = 3,
          .binding = 0,
          .format = vk::Format::eR8G8B8A8Unorm,
          .offset = offsetof(VolumeVertex, weights)
        }
      }};
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 1> getPositionAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(VolumeVertex, position)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_VOLUMEVERTEX_H
