#ifndef VKE_BUFFERS_H
#define VKE_BUFFERS_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class LogicalDevice;

  namespace Buffers {
    void createBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                    const VkQueue& queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

    void endSingleTimeCommands(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool,
                               VkQueue queue, VkCommandBuffer commandBuffer);
  }

} // namespace vke


#endif //VKE_BUFFERS_H
