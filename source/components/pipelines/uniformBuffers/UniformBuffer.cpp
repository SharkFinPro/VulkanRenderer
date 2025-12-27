#include "UniformBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cstring>

namespace vke {

  UniformBuffer::UniformBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkDeviceSize bufferSize)
    : m_logicalDevice(logicalDevice), m_bufferSize(bufferSize)
  {
    const auto maxFramesInFlight = logicalDevice->getMaxFramesInFlight();

    m_uniformBuffers.resize(maxFramesInFlight);
    m_uniformBuffersMemory.resize(maxFramesInFlight);
    m_uniformBuffersMapped.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
      Buffers::createBuffer(logicalDevice, bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            m_uniformBuffers[i], m_uniformBuffersMemory[i]);

      logicalDevice->mapMemory(m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);

      const VkDescriptorBufferInfo bufferInfo {
        .buffer = m_uniformBuffers[i],
        .offset = 0,
        .range = bufferSize
      };

      m_bufferInfos.push_back(bufferInfo);
    }

    m_poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_poolSize.descriptorCount = maxFramesInFlight;
  }

  UniformBuffer::~UniformBuffer()
  {
    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
    {
      m_logicalDevice->unmapMemory(m_uniformBuffersMemory[i]);

      Buffers::destroyBuffer(m_logicalDevice, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
  }

  VkDescriptorPoolSize UniformBuffer::getDescriptorPoolSize() const
  {
    return m_poolSize;
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
      .pBufferInfo = &m_bufferInfos[frame]
    };

    return uniformDescriptorSet;
  }

  void UniformBuffer::update(const uint32_t frame, const void* data) const
  {
    memcpy(m_uniformBuffersMapped[frame], data, m_bufferSize);
  }

} // namespace vke