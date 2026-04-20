#include "SmokeVolume.h"

namespace vke {

  SmokeVolume::SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool& commandPool)
    : ProceduralVolume(std::move(logicalDevice), commandPool)
  {}

} // vke