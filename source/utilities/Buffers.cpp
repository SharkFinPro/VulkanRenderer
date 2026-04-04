#include "Buffers.h"
#include "../components/commandBuffer/SingleUseCommandBuffer.h"
#include "../components/logicalDevice/LogicalDevice.h"
#include "../components/physicalDevice/PhysicalDevice.h"

namespace vke::Buffers {

    void createBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      const vk::DeviceSize size,
                      const vk::BufferUsageFlags usage,
                      const vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer& buffer,
                      vk::raii::DeviceMemory& bufferMemory)
    {
      const vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
      };

      buffer = logicalDevice->createBuffer(bufferInfo);

      const vk::MemoryRequirements memoryRequirements = logicalDevice->getBufferMemoryRequirements(buffer);

      const vk::MemoryAllocateFlagsInfo allocateFlagsInfo {
        .flags = (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
                   ? vk::MemoryAllocateFlagBits::eDeviceAddress
                   : vk::MemoryAllocateFlags{}
      };

      const vk::MemoryAllocateInfo allocateInfo {
        .pNext = &allocateFlagsInfo,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = logicalDevice->getPhysicalDevice()->findMemoryType(memoryRequirements.memoryTypeBits, properties)
      };

      logicalDevice->allocateMemory(allocateInfo, bufferMemory);

      logicalDevice->bindBufferMemory(buffer, bufferMemory);
    }

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const vk::CommandPool& commandPool,
                    vk::Queue queue,
                    vk::Buffer srcBuffer,
                    vk::Buffer dstBuffer,
                    const vk::DeviceSize size)
    {
      const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, queue);

      commandBuffer.record([commandBuffer, &srcBuffer, &dstBuffer, size] {
        const vk::BufferCopy copyRegion {
          .size = size
        };

        commandBuffer.copyBuffer(
          srcBuffer,
          dstBuffer,
          { copyRegion }
        );
      });
    }

} // namespace vke::Buffers