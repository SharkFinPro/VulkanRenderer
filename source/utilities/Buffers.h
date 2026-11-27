#ifndef VKE_BUFFERS_H
#define VKE_BUFFERS_H

#include "../components/logicalDevice/PooledMemory.h"
#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>

namespace vke {

  class LogicalDevice;

  namespace Buffers {
    void createBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      vk::DeviceSize size,
                      vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer& buffer,
                      vk::raii::DeviceMemory& bufferMemory);

    // As createBuffer, but the memory comes from the device's pool and returns to it when destroyed (see
    // LogicalDevice::allocatePooledMemory). Declare the memory before the buffer in whatever struct is retired.
    void createPooledBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            vk::DeviceSize size,
                            vk::BufferUsageFlags usage,
                            vk::MemoryPropertyFlags properties,
                            vk::raii::Buffer& buffer,
                            PooledMemory& bufferMemory);

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const vk::CommandPool& commandPool,
                    vk::Queue queue,
                    vk::Buffer srcBuffer,
                    vk::Buffer dstBuffer,
                    vk::DeviceSize size);

    void doMappedMemoryOperation(const vk::raii::DeviceMemory& deviceMemory,
                                 const std::function<void(void* data)>& operationFunction);
  }

} // namespace vke


#endif //VKE_BUFFERS_H