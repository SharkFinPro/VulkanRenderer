#include "DescriptorAllocator.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace vke {

  DescriptorAllocator::DescriptorAllocator(std::shared_ptr<LogicalDevice> logicalDevice,
                                           const uint32_t objectsPerPool)
    : m_logicalDevice(std::move(logicalDevice)), m_objectsPerPool(objectsPerPool)
  {
    createPool();
  }

  DescriptorAllocator::Allocation DescriptorAllocator::allocate(const std::vector<vk::DescriptorSetLayout>& layouts,
                                                                const void* allocationPNext)
  {
    Allocation allocation;

    for (auto& pool : m_pools)
    {
      if (tryAllocate(pool, layouts, allocationPNext, allocation))
      {
        return allocation;
      }
    }

    createPool();

    if (!tryAllocate(m_pools.back(), layouts, allocationPNext, allocation))
    {
      throw std::runtime_error("Failed to allocate descriptor sets from a new pool!");
    }

    return allocation;
  }

  void DescriptorAllocator::free(const vk::DescriptorPool pool,
                                 const std::vector<vk::DescriptorSet>& sets)
  {
    const auto it = std::ranges::find_if(m_pools, [pool](const Pool& candidate) {
      return *candidate.pool == pool;
    });

    if (it == m_pools.end())
    {
      return;
    }

    m_logicalDevice->freeDescriptorSets(pool, sets);

    it->liveSets -= static_cast<uint32_t>(sets.size());

    // Nothing is live, so resetting only undoes fragmentation.
    if (it->liveSets == 0)
    {
      it->pool.reset();
    }
  }

  uint32_t DescriptorAllocator::getMaxSetsPerPool() const
  {
    return m_logicalDevice->getMaxFramesInFlight() * m_objectsPerPool;
  }

  void DescriptorAllocator::createPool()
  {
    const uint32_t maxSets = getMaxSetsPerPool();

    const std::array<vk::DescriptorPoolSize, 3> poolSizes {{
      { vk::DescriptorType::eUniformBuffer, maxSets },
      { vk::DescriptorType::eStorageBuffer, maxSets },
      { vk::DescriptorType::eCombinedImageSampler, maxSets }
    }};

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = maxSets,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_pools.push_back({ m_logicalDevice->createDescriptorPool(poolCreateInfo) });
  }

  bool DescriptorAllocator::tryAllocate(Pool& pool,
                                        const std::vector<vk::DescriptorSetLayout>& layouts,
                                        const void* allocationPNext,
                                        Allocation& allocation) const
  {
    const auto setCount = static_cast<uint32_t>(layouts.size());

    if (pool.liveSets + setCount > getMaxSetsPerPool())
    {
      return false;
    }

    const vk::DescriptorSetAllocateInfo allocateInfo {
      .pNext = allocationPNext,
      .descriptorPool = *pool.pool,
      .descriptorSetCount = setCount,
      .pSetLayouts = layouts.data()
    };

    try
    {
      auto raiiDescriptorSets = m_logicalDevice->allocateDescriptorSets(allocateInfo);

      allocation.pool = *pool.pool;
      allocation.sets.clear();
      allocation.sets.reserve(raiiDescriptorSets.size());

      // The pool owns the sets; they are returned through free().
      for (auto& descriptorSet : raiiDescriptorSets)
      {
        allocation.sets.push_back(descriptorSet.release());
      }
    }
    catch (const vk::SystemError& error)
    {
      // A pool can run out of a descriptor type or be fragmented before it reaches its set limit.
      const auto result = static_cast<vk::Result>(error.code().value());

      if (result != vk::Result::eErrorOutOfPoolMemory && result != vk::Result::eErrorFragmentedPool)
      {
        throw;
      }

      return false;
    }

    pool.liveSets += setCount;

    return true;
  }

} // namespace vke
