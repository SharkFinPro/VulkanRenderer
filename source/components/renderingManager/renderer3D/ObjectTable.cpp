#include "ObjectTable.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace vke {

  ObjectTable::ObjectTable(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)), m_frames(m_logicalDevice->getMaxFramesInFlight())
  {
    for (auto& frame : m_frames)
    {
      allocate(frame.objects, s_minimumCapacity, sizeof(ObjectData));
      allocate(frame.drawSlots, s_minimumCapacity, sizeof(uint32_t));
    }
  }

  bool ObjectTable::reserve(const uint32_t frame,
                            const size_t objectCount,
                            const size_t slotCount)
  {
    auto& table = m_frames[frame];

    const bool objectsReplaced = grow(table.objects, objectCount, sizeof(ObjectData));
    const bool slotsReplaced = grow(table.drawSlots, slotCount, sizeof(uint32_t));

    return objectsReplaced || slotsReplaced;
  }

  ObjectData* ObjectTable::getEntries(const uint32_t frame) const
  {
    return static_cast<ObjectData*>(m_frames[frame].objects.mapped);
  }

  uint32_t* ObjectTable::getDrawSlots(const uint32_t frame) const
  {
    return static_cast<uint32_t*>(m_frames[frame].drawSlots.mapped);
  }

  const vk::DescriptorBufferInfo& ObjectTable::getBufferInfo(const uint32_t frame) const
  {
    return m_frames[frame].objects.bufferInfo;
  }

  const vk::DescriptorBufferInfo& ObjectTable::getDrawSlotsBufferInfo(const uint32_t frame) const
  {
    return m_frames[frame].drawSlots.bufferInfo;
  }

  size_t ObjectTable::getCapacity(const uint32_t frame) const
  {
    return m_frames[frame].objects.capacity;
  }

  size_t ObjectTable::getDrawSlotCapacity(const uint32_t frame) const
  {
    return m_frames[frame].drawSlots.capacity;
  }

  bool ObjectTable::grow(MappedBuffer& target,
                         const size_t count,
                         const size_t elementSize) const
  {
    if (count <= target.capacity)
    {
      return false;
    }

    // Retired rather than destroyed, so nothing here depends on which frames could still be using it. Freeing the
    // memory unmaps it.
    m_logicalDevice->retireHandles(std::move(target));

    target = {};

    allocate(target, std::bit_ceil(std::max(count, s_minimumCapacity)), elementSize);

    return true;
  }

  void ObjectTable::allocate(MappedBuffer& target,
                             const size_t capacity,
                             const size_t elementSize) const
  {
    const vk::DeviceSize size = capacity * elementSize;

    constexpr auto usage = vk::BufferUsageFlagBits::eStorageBuffer;
    constexpr auto hostAccess = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    // Device-local memory the host can write (resizable BAR, unified memory) puts the data where the GPU reads it
    // fastest; otherwise plain host memory.
    try
    {
      Buffers::createBuffer(m_logicalDevice, size, usage, hostAccess | vk::MemoryPropertyFlagBits::eDeviceLocal,
                            target.buffer, target.memory);
    }
    catch (const std::runtime_error&)
    {
      Buffers::createBuffer(m_logicalDevice, size, usage, hostAccess, target.buffer, target.memory);
    }

    target.mapped = target.memory.mapMemory(0, vk::WholeSize);
    target.capacity = capacity;
    const vk::DescriptorBufferInfo bufferInfo {
      .buffer = *target.buffer,
      .offset = 0,
      .range = size
    };

    target.bufferInfo = bufferInfo;
  }

} // namespace vke
