#include "PooledMemory.h"
#include "LogicalDevice.h"
#include <utility>

namespace vke {

  PooledMemory::PooledMemory(LogicalDevice* device,
                             const PooledMemoryKey& key,
                             PooledBlock&& block)
    : m_device(device),
      m_key(key),
      m_block(std::move(block))
  {
  }

  PooledMemory::PooledMemory(PooledMemory&& other) noexcept
    : m_device(std::exchange(other.m_device, nullptr)),
      m_key(other.m_key),
      m_block(std::move(other.m_block))
  {
    other.m_block.mapped = nullptr;
  }

  PooledMemory& PooledMemory::operator=(PooledMemory&& other) noexcept
  {
    if (this != &other)
    {
      release();

      m_device = std::exchange(other.m_device, nullptr);
      m_key = other.m_key;
      m_block = std::move(other.m_block);
      other.m_block.mapped = nullptr;
    }

    return *this;
  }

  PooledMemory::~PooledMemory()
  {
    release();
  }

  const vk::raii::DeviceMemory& PooledMemory::get() const
  {
    return m_block.memory;
  }

  void* PooledMemory::map()
  {
    if (!m_block.mapped)
    {
      m_block.mapped = m_block.memory.mapMemory(0, vk::WholeSize);
    }

    return m_block.mapped;
  }

  void PooledMemory::release()
  {
    if (m_device && *m_block.memory)
    {
      m_device->releaseToPool(m_key, std::move(m_block));
    }

    m_block = {};
    m_device = nullptr;
  }

} // namespace vke
