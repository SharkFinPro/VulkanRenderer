#ifndef VULKANPROJECT_UNIFORMBUFFER_H
#define VULKANPROJECT_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class LogicalDevice;

class UniformBuffer {
public:
  UniformBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDeviceSize bufferSize);
  ~UniformBuffer();

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize() const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet, size_t frame) const;

  void update(uint32_t frame, const void* data) const;

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::vector<VkBuffer> m_uniformBuffers;
  std::vector<VkDeviceMemory> m_uniformBuffersMemory;
  std::vector<void*> m_uniformBuffersMapped;

  std::vector<VkDescriptorBufferInfo> m_bufferInfos;
  VkDescriptorPoolSize m_poolSize{};

  VkDeviceSize m_bufferSize;
};


#endif //VULKANPROJECT_UNIFORMBUFFER_H
