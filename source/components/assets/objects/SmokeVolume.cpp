#include "SmokeVolume.h"

namespace vke {

  SmokeVolume::SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool& commandPool,
                           glm::vec3 position)
    : ProceduralVolume(
        std::move(logicalDevice),
        commandPool,
        {
          -1.f, 0.f, -1.f,
          1.f, 1.f, 1.f
        },
        position,
        glm::vec3(1.5f, 15.0f, 1.5f))
  {}

} // vke