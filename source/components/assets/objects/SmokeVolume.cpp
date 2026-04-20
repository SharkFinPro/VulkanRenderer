#include "SmokeVolume.h"

namespace vke {

  SmokeVolume::SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool& commandPool,
                           glm::vec3 position)
    : ProceduralVolume(
        std::move(logicalDevice),
        commandPool,
        {-1, -1, -1, 1, 10, 1},
        position)
  {}

} // vke