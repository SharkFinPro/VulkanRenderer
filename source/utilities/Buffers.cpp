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

    const vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();

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

    buffer.bindMemory(bufferMemory, 0);
  }

  void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const vk::CommandPool& commandPool,
                  vk::Queue queue,
                  vk::Buffer srcBuffer,
                  vk::Buffer dstBuffer,
                  const vk::DeviceSize size)
  {
    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, queue);

    commandBuffer.record([&commandBuffer, &srcBuffer, &dstBuffer, size] {
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

  void doMappedMemoryOperation(const vk::raii::DeviceMemory& deviceMemory,
                               const std::function<void(void* data)>& operationFunction)
  {
    void* data = deviceMemory.mapMemory(0, vk::WholeSize);

    operationFunction(data);

    deviceMemory.unmapMemory();
  }

} // namespace vke::Buffers