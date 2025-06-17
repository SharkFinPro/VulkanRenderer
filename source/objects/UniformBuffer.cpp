#include "UniformBuffer.h"
#include "../utilities/Buffers.h"
#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"

UniformBuffer::UniformBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<PhysicalDevice>& physicalDevice, const VkDeviceSize bufferSize)
  : logicalDevice(logicalDevice), bufferSize(bufferSize)
{
  uniformBuffers.resize(logicalDevice->getMaxFramesInFlight());
  uniformBuffersMemory.resize(logicalDevice->getMaxFramesInFlight());
  uniformBuffersMapped.resize(logicalDevice->getMaxFramesInFlight());

  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::createBuffer(logicalDevice, physicalDevice, bufferSize,
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(logicalDevice->getDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

    const VkDescriptorBufferInfo bufferInfo {
      .buffer = uniformBuffers[i],
      .offset = 0,
      .range = bufferSize
    };

    bufferInfos.push_back(bufferInfo);
  }

  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = logicalDevice->getMaxFramesInFlight();
}

UniformBuffer::~UniformBuffer()
{
  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    if (uniformBuffersMapped[i] != VK_NULL_HANDLE)
    {
      vkUnmapMemory(logicalDevice->getDevice(), uniformBuffersMemory[i]);
    }

    Buffers::destroyBuffer(logicalDevice, uniformBuffers[i], uniformBuffersMemory[i]);
  }
}

VkDescriptorPoolSize UniformBuffer::getDescriptorPoolSize() const
{
  return poolSize;
}

VkWriteDescriptorSet UniformBuffer::getDescriptorSet(const uint32_t binding, const VkDescriptorSet& dstSet,
                                                     const size_t frame) const
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

void UniformBuffer::update(const uint32_t frame, const void* data) const
{
  memcpy(uniformBuffersMapped[frame], data, bufferSize);
}
