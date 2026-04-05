#include "SingleUseCommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {
  SingleUseCommandBuffer::SingleUseCommandBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                 const vk::CommandPool& commandPool,
                                                 const vk::Queue queue)
    : m_queue(queue)
  {
    SingleUseCommandBuffer::allocateCommandBuffers(logicalDevice, commandPool);
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

  void SingleUseCommandBuffer::allocateCommandBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                      const vk::CommandPool commandPool)
  {
    const vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1
    };

    logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers);
  }
} // vke