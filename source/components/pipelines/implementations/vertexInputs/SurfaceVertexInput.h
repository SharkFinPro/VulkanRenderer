#ifndef VKE_SURFACEVERTEXINPUT_H
#define VKE_SURFACEVERTEXINPUT_H

#include "../../../assets/volume/SurfaceExtractor.h"
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>

namespace vke {

  // How the volume surface pipeline reads a SurfaceVertex.
  struct SurfaceVertexInput {
    static constexpr vk::VertexInputBindingDescription getBindingDescription()
    {
      return {
        .binding = 0,
        .stride = sizeof(SurfaceVertex),
        .inputRate = vk::VertexInputRate::eVertex
      };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
      return {{
        {
          .location = 0,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(SurfaceVertex, position)
        },
        {
          .location = 1,
          .binding = 0,
          .format = vk::Format::eR32G32B32Sfloat,
          .offset = offsetof(SurfaceVertex, normal)
        },
        {
          .location = 2,
          .binding = 0,
          .format = vk::Format::eR32Uint,
          .offset = offsetof(SurfaceVertex, material)
        }
      }};
    }
  };

} // namespace vke

#endif //VKE_SURFACEVERTEXINPUT_H
