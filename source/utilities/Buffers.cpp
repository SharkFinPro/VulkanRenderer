#include "Buffers.h"
#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"
#include <stdexcept>

namespace Buffers {
  void createBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const std::shared_ptr<PhysicalDevice>& physicalDevice, const VkDeviceSize size,
                    const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory)
  {
    const VkBufferCreateInfo bufferInfo {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(logicalDevice->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(logicalDevice->getDevice(), buffer, &memoryRequirements);

    const VkMemoryAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = physicalDevice->findMemoryType(memoryRequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(logicalDevice->getDevice(), &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(logicalDevice->getDevice(), buffer, bufferMemory, 0);
  }

  void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                  const VkQueue& queue, const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
  {
    const VkCommandBuffer commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);

    const VkBufferCopy copyRegion {
      .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(logicalDevice, commandPool, queue, commandBuffer);
  }

  void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
  {
    if (bufferMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory(logicalDevice->getDevice(), bufferMemory, nullptr);
      bufferMemory = VK_NULL_HANDLE;
    }

    if (buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(logicalDevice->getDevice(), buffer, nullptr);
      buffer = VK_NULL_HANDLE;
    }
  }

  VkCommandBuffer beginSingleTimeCommands(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool)
  {
    const VkCommandBufferAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate command buffer!");
    }

    constexpr VkCommandBufferBeginInfo beginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to begin command buffer!");
    }

    return commandBuffer;
  }

  void endSingleTimeCommands(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool,
                             VkQueue queue, VkCommandBuffer commandBuffer)
  {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to end command buffer!");
    }

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer
    };

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit command buffer!");
    }

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(logicalDevice->getDevice(), commandPool, 1, &commandBuffer);
  }
}