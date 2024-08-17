#ifndef VULKANPROJECT_UNIFORMBUFFER_H
#define VULKANPROJECT_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class UniformBuffer {
public:
  UniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, uint32_t MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize);
  ~UniformBuffer();

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize() const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet, size_t frame) const;

  void update(uint32_t frame, const void* data, size_t size) const;

protected:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  uint32_t MAX_FRAMES_IN_FLIGHT;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  std::vector<VkDescriptorBufferInfo> bufferInfos;
  VkDescriptorPoolSize poolSize;
};


#endif //VULKANPROJECT_UNIFORMBUFFER_H
