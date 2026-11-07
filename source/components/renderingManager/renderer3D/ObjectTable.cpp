#include "ObjectTable.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace vke {

  ObjectTable::ObjectTable(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)), m_frames(m_logicalDevice->getMaxFramesInFlight())
  {
    for (auto& frame : m_frames)
    {
      allocate(frame, s_minimumCapacity);
    }
  }

  bool ObjectTable::reserve(const uint32_t frame,
                            const size_t count)
  {
    auto& table = m_frames[frame];

    if (count <= table.capacity)
    {
      return false;
    }

    // Retired rather than destroyed, so nothing here depends on which frames could still be using it. Freeing the
    // memory unmaps it.
    m_logicalDevice->retireHandles(std::move(table));

    table = {};

    allocate(table, std::bit_ceil(std::max(count, s_minimumCapacity)));

    return true;
  }

  ObjectData* ObjectTable::getEntries(const uint32_t frame) const
  {
    return m_frames[frame].entries;
  }

  const vk::DescriptorBufferInfo& ObjectTable::getBufferInfo(const uint32_t frame) const
  {
    return m_frames[frame].bufferInfo;
  }

  size_t ObjectTable::getCapacity(const uint32_t frame) const
  {
    return m_frames[frame].capacity;
  }

  void ObjectTable::allocate(FrameTable& frame,
                             const size_t capacity) const
  {
    const vk::DeviceSize size = capacity * sizeof(ObjectData);

    constexpr auto usage = vk::BufferUsageFlagBits::eStorageBuffer;
    constexpr auto hostAccess = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    // Device-local memory the host can write (resizable BAR, unified memory) puts the table where the GPU reads it
    // fastest; otherwise plain host memory.
    try
    {
      Buffers::createBuffer(m_logicalDevice, size, usage, hostAccess | vk::MemoryPropertyFlagBits::eDeviceLocal,
                            frame.buffer, frame.memory);
    }
    catch (const std::runtime_error&)
    {
      Buffers::createBuffer(m_logicalDevice, size, usage, hostAccess, frame.buffer, frame.memory);
    }

    frame.entries = static_cast<ObjectData*>(frame.memory.mapMemory(0, vk::WholeSize));
    frame.capacity = capacity;
    const vk::DescriptorBufferInfo bufferInfo {
      .buffer = *frame.buffer,
      .offset = 0,
      .range = size
    };

    frame.bufferInfo = bufferInfo;
  }

} // namespace vke
