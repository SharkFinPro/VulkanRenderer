#include "Buffers.h"
#include "../components/logicalDevice/LogicalDevice.h"
#include "../components/physicalDevice/PhysicalDevice.h"
#include <stdexcept>

namespace vke::Buffers {

    void createBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      const VkDeviceSize size,
                      const VkBufferUsageFlags usage,
                      const VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory)
    {
      const VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
      };

      buffer = logicalDevice->createBuffer(bufferInfo);

      const VkMemoryRequirements memoryRequirements = logicalDevice->getBufferMemoryRequirements(buffer);

      const VkMemoryAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = logicalDevice->getPhysicalDevice()->findMemoryType(memoryRequirements.memoryTypeBits, properties)
      };

      logicalDevice->allocateMemory(allocateInfo, bufferMemory);

      logicalDevice->bindBufferMemory(buffer, bufferMemory);
    }

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const VkCommandPool& commandPool,
                    const VkQueue& queue,
                    const VkBuffer srcBuffer,
                    const VkBuffer dstBuffer,
                    const VkDeviceSize size)
    {
      const VkCommandBuffer commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);

      const VkBufferCopy copyRegion {
        .size = size
      };
      vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

      endSingleTimeCommands(logicalDevice, commandPool, queue, commandBuffer);
    }

    void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory)
    {
      logicalDevice->freeMemory(bufferMemory);

      logicalDevice->destroyBuffer(buffer);
    }

} // namespace vke::Buffers