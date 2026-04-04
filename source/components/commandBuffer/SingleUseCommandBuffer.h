#ifndef VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H
#define VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H

#include "CommandBuffer.h"
#include <vulkan/vulkan_raii.hpp>

namespace vke {

  class SingleUseCommandBuffer final : public CommandBuffer {
  public:
    SingleUseCommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool& commandPool,
                           vk::Queue& queue);

    void record(const std::function<void()>& renderFunction) const override;

  private:
    vk::Queue& m_queue;

    void allocateCommandBuffers(const vk::CommandPool& commandPool) override;
  };

} // vke

#endif //VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H