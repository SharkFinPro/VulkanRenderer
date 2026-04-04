#ifndef VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H
#define VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H

#include "CommandBuffer.h"
#include <vulkan/vulkan_raii.hpp>

namespace vke {

  class SingleUseCommandBuffer final : public CommandBuffer {
  public:
    SingleUseCommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                           vk::raii::CommandPool& commandPool,
                           vk::raii::Queue& queue);

    void record(const std::function<void()>& renderFunction) const override;

  private:
    vk::raii::CommandPool& m_commandPool;

    vk::raii::Queue& m_queue;

    void allocateCommandBuffers(vk::raii::CommandPool& commandPool) override;
  };

} // vke

#endif //VULKANPROJECT_SINGLEUSECOMMANDBUFFER_H