#ifndef VKE_POOLEDMEMORYKEY_H
#define VKE_POOLEDMEMORYKEY_H

#include <vulkan/vulkan_raii.hpp>
#include <compare>
#include <cstdint>

namespace vke {

  // Identifies a block of pooled device memory: blocks with equal keys are interchangeable.
  struct PooledMemoryKey {
    uint32_t memoryType = 0;
    vk::DeviceSize size = 0;
    bool deviceAddress = false;

    auto operator<=>(const PooledMemoryKey&) const = default;
  };

} // namespace vke

#endif //VKE_POOLEDMEMORYKEY_H
