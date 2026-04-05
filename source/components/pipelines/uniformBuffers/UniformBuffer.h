#ifndef VKE_UNIFORMBUFFER_H
#define VKE_UNIFORMBUFFER_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class UniformBuffer {
  public:
    UniformBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                  vk::DeviceSize bufferSize);

    ~UniformBuffer();

    [[nodiscard]] vk::WriteDescriptorSet getDescriptorSet(uint32_t binding,
                                                          vk::DescriptorSet dstSet,
                                                          size_t frame) const;

    void update(uint32_t frame,
                const void* data) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<vk::raii::Buffer> m_uniformBuffers;
    std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    std::vector<vk::DescriptorBufferInfo> m_bufferInfos;

    vk::DeviceSize m_bufferSize;
  };

} // namespace vke

#endif //VKE_UNIFORMBUFFER_H