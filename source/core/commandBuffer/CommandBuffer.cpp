#include "CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include <stdexcept>

CommandBuffer::CommandBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool)
  : logicalDevice(logicalDevice)
{
  allocateCommandBuffers(commandPool);
}

void CommandBuffer::record(const uint32_t currentFrame,
                           const std::function<void(const VkCommandBuffer& cmdBuffer)>& renderFunction) const
{
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  };

  if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  renderFunction(commandBuffers[currentFrame]);

  if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void CommandBuffer::resetCommandBuffer(const uint32_t currentFrame) const
{
  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
}

VkCommandBuffer* CommandBuffer::getCommandBuffer(const uint32_t currentFrame)
{
  return &commandBuffers[currentFrame];
}

void CommandBuffer::allocateCommandBuffers(VkCommandPool commandPool)
{
  commandBuffers.resize(logicalDevice->getMaxFramesInFlight());

  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
  };

  logicalDevice->allocateCommandBuffers(allocInfo, commandBuffers.data());
}
