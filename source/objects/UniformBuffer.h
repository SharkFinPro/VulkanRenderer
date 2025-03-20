#ifndef VULKANPROJECT_UNIFORMBUFFER_H
#define VULKANPROJECT_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class LogicalDevice;
class PhysicalDevice;

class UniformBuffer {
public:
  UniformBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<PhysicalDevice>& physicalDevice, VkDeviceSize bufferSize);
  ~UniformBuffer();

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize() const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet, size_t frame) const;

  void update(uint32_t frame, const void* data) const;

protected:
  std::shared_ptr<LogicalDevice> logicalDevice;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  std::vector<VkDescriptorBufferInfo> bufferInfos;
  VkDescriptorPoolSize poolSize{};

  VkDeviceSize bufferSize;
};


#endif //VULKANPROJECT_UNIFORMBUFFER_H
