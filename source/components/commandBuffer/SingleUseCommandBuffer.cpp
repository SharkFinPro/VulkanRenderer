#include "SingleUseCommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include <stdexcept>

namespace vke {
  SingleUseCommandBuffer::SingleUseCommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                                                 VkCommandPool commandPool,
                                                 VkQueue queue)
    : CommandBuffer(std::move(logicalDevice)), m_commandPool(commandPool), m_queue(queue)
  {
    SingleUseCommandBuffer::allocateCommandBuffers(VK_NULL_HANDLE);
  }

  void SingleUseCommandBuffer::record(const std::function<void()>& renderFunction) const
  {
    constexpr VkCommandBufferBeginInfo beginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to begin command buffer!");
    }

    renderFunction();

    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to end command buffer!");
    }

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &m_commandBuffers[m_currentFrame]
    };

    if (vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit command buffer!");
    }

    vkQueueWaitIdle(m_queue);

    m_logicalDevice->freeCommandBuffers(m_commandPool, 1, m_commandBuffers.data());
  }

  void SingleUseCommandBuffer::allocateCommandBuffers(VkCommandPool commandPool)
  {
    m_commandBuffers.resize(1);

    const VkCommandBufferAllocateInfo allocInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    m_logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers.data());
  }
} // vke