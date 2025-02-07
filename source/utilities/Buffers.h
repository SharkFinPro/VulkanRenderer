#ifndef VULKANPROJECT_BUFFERS_H
#define VULKANPROJECT_BUFFERS_H

#include <vulkan/vulkan.h>
#include <memory>

class LogicalDevice;

namespace Buffers {

  struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
  };

  uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

  void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkDeviceSize size,
                    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory);

  void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue, VkBuffer srcBuffer,
                  VkBuffer dstBuffer, VkDeviceSize size);

  void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, const Buffer& buffer);

  VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool);

  void endSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                             VkCommandBuffer commandBuffer);
};


#endif //VULKANPROJECT_BUFFERS_H
