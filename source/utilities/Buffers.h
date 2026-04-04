#ifndef VKE_BUFFERS_H
#define VKE_BUFFERS_H

#include <vulkan/vulkan_raii.hpp>
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

    void copyBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const vk::CommandPool& commandPool,
                    vk::Queue& queue,
                    vk::Buffer srcBuffer,
                    vk::Buffer dstBuffer,
                    vk::DeviceSize size);
  }

} // namespace vke


#endif //VKE_BUFFERS_H