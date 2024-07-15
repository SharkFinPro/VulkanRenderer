#include "UniformBuffer.h"
#include <cstring>
#include "../utilities/Buffers.h"

UniformBuffer::UniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, uint32_t MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize)
  : device(device), physicalDevice(physicalDevice), MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT)
{
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;

    bufferInfos.push_back(bufferInfo);
  }

  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
}

UniformBuffer::~UniformBuffer() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }
}

VkDescriptorPoolSize UniformBuffer::getDescriptorPoolSize() const
{
  return poolSize;
}

VkWriteDescriptorSet UniformBuffer::getDescriptorSet(uint32_t binding, VkDescriptorSet& dstSet, size_t frame) const
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

void UniformBuffer::update(uint32_t frame, void* data, size_t size)
{
  memcpy(uniformBuffersMapped[frame], data, size);
}
