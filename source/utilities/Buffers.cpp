#include "Buffers.h"
#include <stdexcept>

uint32_t Buffers::findMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t typeFilter,
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

void Buffers::createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size,
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

void Buffers::copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue, const VkBuffer srcBuffer,
                         const VkBuffer dstBuffer, const VkDeviceSize size)
{
  const VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

VkCommandBuffer Buffers::beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool)
{
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandPool = commandPool;
  allocateInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

  VkCommandBufferBeginInfo  beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Buffers::endSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                                    const VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}