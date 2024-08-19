#include "UniformBuffer.h"
#include <cstring>
#include "../utilities/Buffers.h"

UniformBuffer::UniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, const uint32_t MAX_FRAMES_IN_FLIGHT, const VkDeviceSize bufferSize)
  : device(device), physicalDevice(physicalDevice), MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT)
{
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

    const VkDescriptorBufferInfo bufferInfo {
      .buffer = uniformBuffers[i],
      .offset = 0,
      .range = bufferSize
    };

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

VkWriteDescriptorSet UniformBuffer::getDescriptorSet(const uint32_t binding, const VkDescriptorSet& dstSet, const size_t frame) const
{
  const VkWriteDescriptorSet uniformDescriptorSet {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = dstSet,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .pBufferInfo = &bufferInfos[frame]
  };

  return uniformDescriptorSet;
}

void UniformBuffer::update(const uint32_t frame, const void* data, const size_t size) const
{
  memcpy(uniformBuffersMapped[frame], data, size);
}
