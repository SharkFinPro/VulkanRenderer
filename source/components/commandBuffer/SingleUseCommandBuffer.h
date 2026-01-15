#ifndef VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H
#define VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H

#include "CommandBuffer.h"
#include <vulkan/vulkan.h>

namespace vke {

  class SingleUseCommandBuffer final : public CommandBuffer {
  public:
    SingleUseCommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                           VkCommandPool commandPool,
                           VkQueue queue);

    void record(const std::function<void()>& renderFunction) const override;

  private:
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkQueue m_queue = VK_NULL_HANDLE;

    void allocateCommandBuffers(VkCommandPool commandPool) override;
  };

} // vke

#endif //VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H