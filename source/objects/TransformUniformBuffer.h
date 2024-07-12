#ifndef VULKANPROJECT_TRANSFORMUNIFORMBUFFER_H
#define VULKANPROJECT_TRANSFORMUNIFORMBUFFER_H

#include "UniformBuffer.h"

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class TransformUniformBuffer : public UniformBuffer {
public:
  TransformUniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, uint32_t MAX_FRAMES_IN_FLIGHT);

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, VkDescriptorSet &dstSet, size_t frame) const override;

  void update(uint32_t frame, TransformUniform& transformUniform);
};


#endif //VULKANPROJECT_TRANSFORMUNIFORMBUFFER_H
