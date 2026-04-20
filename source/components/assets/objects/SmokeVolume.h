#ifndef VULKANPROJECT_SMOKEVOLUME_H
#define VULKANPROJECT_SMOKEVOLUME_H

#include "ProceduralVolume.h"


namespace vke {

  class SmokeVolume : public ProceduralVolume {
  public:
    SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                const vk::CommandPool& commandPool,
                glm::vec3 position);
  };

} // vke

#endif //VULKANPROJECT_SMOKEVOLUME_H
