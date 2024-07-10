#ifndef VULKANPROJECT_BUFFERS_H
#define VULKANPROJECT_BUFFERS_H

#include <vulkan/vulkan.h>

class Buffers {
public:
  static uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties);

  static void createBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDeviceSize size,
                           VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                           VkDeviceMemory& bufferMemory);

  static void copyBuffer(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue, VkBuffer srcBuffer,
                         VkBuffer dstBuffer, VkDeviceSize size);

  static VkCommandBuffer beginSingleTimeCommands(VkDevice& device, VkCommandPool& commandPool);

  static void endSingleTimeCommands(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue,
                                    VkCommandBuffer commandBuffer);
};


#endif //VULKANPROJECT_BUFFERS_H
