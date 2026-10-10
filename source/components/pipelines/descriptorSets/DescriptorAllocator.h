#ifndef VKE_DESCRIPTORALLOCATOR_H
#define VKE_DESCRIPTORALLOCATOR_H

#include <vulkan/vulkan_raii.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  // Hands out descriptor sets from pools that can take them back, so pools are reused as their sets are
  // freed and a new one is only created when none has room.
  class DescriptorAllocator {
  public:
    struct Allocation {
      std::vector<vk::DescriptorSet> sets;
      vk::DescriptorPool pool;
    };

    explicit DescriptorAllocator(LogicalDevice& logicalDevice,
                                 uint32_t objectsPerPool = 500);

    [[nodiscard]] Allocation allocate(const std::vector<vk::DescriptorSetLayout>& layouts,
                                      const void* allocationPNext = nullptr);

    // The caller guarantees the GPU no longer uses the sets.
    void free(vk::DescriptorPool pool,
              const std::vector<vk::DescriptorSet>& sets);

  private:
    struct Pool {
      vk::raii::DescriptorPool pool;
      uint32_t liveSets = 0;
      bool full = false;
    };

    // Not owning: the device's deferred queue can hold this allocator, and an owning pointer would make a cycle.
    // Callers keep the device alive, and ~LogicalDevice drains the queue before m_device goes.
    LogicalDevice* m_logicalDevice;

    std::vector<Pool> m_pools;
    uint32_t m_objectsPerPool;

    [[nodiscard]] uint32_t getMaxSetsPerPool() const;

    void createPool();

    [[nodiscard]] bool tryAllocate(Pool& pool,
                                   const std::vector<vk::DescriptorSetLayout>& layouts,
                                   const void* allocationPNext,
                                   Allocation& allocation) const;
  };

} // namespace vke

#endif //VKE_DESCRIPTORALLOCATOR_H
