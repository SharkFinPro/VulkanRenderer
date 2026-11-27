#ifndef VKE_POOLEDMEMORY_H
#define VKE_POOLEDMEMORY_H

#include "PooledMemoryKey.h"
#include <vulkan/vulkan_raii.hpp>

namespace vke {

  class LogicalDevice;

  // A block of device memory and, once map() has been called, its persistent mapping.
  struct PooledBlock {
    vk::raii::DeviceMemory memory = nullptr;
    void* mapped = nullptr;
  };

  // Device memory that goes back to its device's pool instead of being freed when this is destroyed, so a buffer that
  // is created and dropped often skips the driver's allocation. Like any memory, destroy it only once the GPU is done
  // with it: held in a struct that is retired, it is destroyed after the buffers declared after it.
  class PooledMemory {
  public:
    PooledMemory() = default;

    PooledMemory(LogicalDevice* device,
                 const PooledMemoryKey& key,
                 PooledBlock&& block);

    PooledMemory(PooledMemory&& other) noexcept;

    PooledMemory& operator=(PooledMemory&& other) noexcept;

    PooledMemory(const PooledMemory&) = delete;

    PooledMemory& operator=(const PooledMemory&) = delete;

    ~PooledMemory();

    [[nodiscard]] const vk::raii::DeviceMemory& get() const;

    // Maps the whole block the first time and keeps it mapped, also while the block waits in the pool. The memory must
    // be host visible and coherent.
    [[nodiscard]] void* map();

  private:
    LogicalDevice* m_device = nullptr;
    PooledMemoryKey m_key;
    PooledBlock m_block;

    void release();
  };

} // namespace vke

#endif //VKE_POOLEDMEMORY_H
