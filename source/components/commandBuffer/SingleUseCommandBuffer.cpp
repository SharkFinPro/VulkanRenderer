#include "SingleUseCommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {
  SingleUseCommandBuffer::SingleUseCommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                                                 const vk::CommandPool& commandPool,
                                                 const vk::Queue queue)
    : CommandBuffer(std::move(logicalDevice)), m_queue(queue)
  {
    SingleUseCommandBuffer::allocateCommandBuffers(commandPool);
  }

  void SingleUseCommandBuffer::record(const std::function<void()>& renderFunction) const
  {
    constexpr vk::CommandBufferBeginInfo beginInfo {
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    m_commandBuffers[m_currentFrame].begin(beginInfo);

    renderFunction();

    m_commandBuffers[m_currentFrame].end();

    const vk::SubmitInfo submitInfo {
      .commandBufferCount = 1,
      .pCommandBuffers = &*m_commandBuffers[m_currentFrame]
    };

    m_queue.submit({ submitInfo });
    m_queue.waitIdle();
  }

  void SingleUseCommandBuffer::allocateCommandBuffers(const vk::CommandPool commandPool)
  {
    const vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1
    };

    m_logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers);
  }
} // vke