#include "TransformUniformBuffer.h"
#include <cstring>

TransformUniformBuffer::TransformUniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice,
                                               uint32_t MAX_FRAMES_IN_FLIGHT)
  : UniformBuffer(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(TransformUniform))
{}

VkWriteDescriptorSet TransformUniformBuffer::getDescriptorSet(uint32_t binding, VkDescriptorSet& dstSet, size_t frame) const
{
  VkWriteDescriptorSet uniformDescriptorSet{};
  uniformDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  uniformDescriptorSet.dstSet = dstSet;
  uniformDescriptorSet.dstBinding = binding;
  uniformDescriptorSet.dstArrayElement = 0;
  uniformDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformDescriptorSet.descriptorCount = 1;
  uniformDescriptorSet.pBufferInfo = &bufferInfos[frame];

  return uniformDescriptorSet;
}

void TransformUniformBuffer::update(uint32_t frame, TransformUniform& transformUniform)
{
  memcpy(uniformBuffersMapped[frame], &transformUniform, sizeof(transformUniform));
}
