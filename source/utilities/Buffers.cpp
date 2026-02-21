#include "Buffers.h"
#include "../components/commandBuffer/SingleUseCommandBuffer.h"
#include "../components/logicalDevice/LogicalDevice.h"
#include "../components/physicalDevice/PhysicalDevice.h"

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

      const VkMemoryAllocateFlagsInfo allocateFlagsInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : static_cast<VkMemoryAllocateFlags>(0)
      };

      const VkMemoryAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &allocateFlagsInfo,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = logicalDevice->getPhysicalDevice()->findMemoryType(memoryRequirements.memoryTypeBits, properties)
      };

      logicalDevice->allocateMemory(allocateInfo, bufferMemory);

      logicalDevice->bindBufferMemory(buffer, bufferMemory);
    }

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    VkCommandPool commandPool,
                    VkQueue queue,
                    VkBuffer srcBuffer,
                    VkBuffer dstBuffer,
                    const VkDeviceSize size)
    {
      const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, queue);

      commandBuffer.record([commandBuffer, srcBuffer, dstBuffer, size] {
        const VkBufferCopy copyRegion {
          .size = size
        };

        commandBuffer.copyBuffer(
          srcBuffer,
          dstBuffer,
          { copyRegion }
        );
      });
    }

    void destroyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory)
    {
      if (bufferMemory != VK_NULL_HANDLE)
      {
        logicalDevice->freeMemory(bufferMemory);

        bufferMemory = VK_NULL_HANDLE;
      }

      if (buffer != VK_NULL_HANDLE)
      {
        logicalDevice->destroyBuffer(buffer);

        buffer = VK_NULL_HANDLE;
      }
    }

} // namespace vke::Buffers