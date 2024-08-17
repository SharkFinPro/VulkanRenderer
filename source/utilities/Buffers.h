#ifndef VULKANPROJECT_BUFFERS_H
#define VULKANPROJECT_BUFFERS_H

#include <vulkan/vulkan.h>

class Buffers {
public:
  static uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties);

  static void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkDeviceSize size,
                           VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                           VkDeviceMemory& bufferMemory);

  static void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue, VkBuffer srcBuffer,
                         VkBuffer dstBuffer, VkDeviceSize size);

  static VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool);

  static void endSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& queue,
                                    VkCommandBuffer commandBuffer);
};


#endif //VULKANPROJECT_BUFFERS_H
