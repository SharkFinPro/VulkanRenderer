#include "UniformBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cstring>

namespace vke {

  UniformBuffer::UniformBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                               const vk::DeviceSize bufferSize)
    : m_logicalDevice(std::move(logicalDevice)), m_bufferSize(bufferSize)
  {
    const auto maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

    m_uniformBuffers.reserve(maxFramesInFlight);
    m_uniformBuffersMemory.reserve(maxFramesInFlight);
    m_uniformBuffersMapped.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
      Buffers::createBuffer(m_logicalDevice, m_bufferSize,
                            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                            m_uniformBuffers.emplace_back(nullptr),
                            m_uniformBuffersMemory.emplace_back(nullptr));

      m_logicalDevice->mapMemory(m_uniformBuffersMemory[i], 0, m_bufferSize,  vk::MemoryMapFlags{}, &m_uniformBuffersMapped[i]);

      m_bufferInfos.push_back({
        .buffer = *m_uniformBuffers[i],
        .offset = 0,
        .range = m_bufferSize
      });
    }
  }

  vk::WriteDescriptorSet UniformBuffer::getDescriptorSet(const uint32_t binding,
                                                         const vk::DescriptorSet dstSet,
                                                         const size_t frame) const
  {
    const vk::WriteDescriptorSet uniformDescriptorSet {
      .dstSet = dstSet,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &m_bufferInfos[frame]
    };

    return uniformDescriptorSet;
  }

  void UniformBuffer::update(const uint32_t frame,
                             const void* data) const
  {
    memcpy(m_uniformBuffersMapped[frame], data, m_bufferSize);
  }

} // namespace vke