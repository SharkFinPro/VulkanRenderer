#include "Buffers.h"
#include "../components/LogicalDevice.h"
#include <stdexcept>

namespace Buffers {
  uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t typeFilter,
                          const VkMemoryPropertyFlags properties)
  {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
      if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size,
                    const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory)
  {
    const VkBufferCreateInfo bufferInfo {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    const VkMemoryAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
  }

  void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                  const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
  {
    const VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    const VkBufferCopy copyRegion {
      .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);
  }

  void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkBuffer buffer, VkDeviceMemory bufferMemory)
  {
    vkDestroyBuffer(logicalDevice->getDevice(), buffer, nullptr);
    vkFreeMemory(logicalDevice->getDevice(), bufferMemory, nullptr);
  }

  VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool)
  {
    const VkCommandBufferAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate command buffer!");
    }

    constexpr VkCommandBufferBeginInfo beginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
  }

  void endSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                             const VkCommandBuffer commandBuffer)
  {
    vkEndCommandBuffer(commandBuffer);

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  }
}