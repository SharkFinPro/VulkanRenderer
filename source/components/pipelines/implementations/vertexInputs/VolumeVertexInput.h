#ifndef VKE_VOLUMEVERTEXINPUT_H
#define VKE_VOLUMEVERTEXINPUT_H

#include "../../../assets/volume/VolumeVertex.h"
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>

namespace vke {

  // How the volume surface pipelines read a VolumeVertex.
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

#endif //VKE_VOLUMEVERTEXINPUT_H
