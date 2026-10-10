#include "DescriptorSet.h"
#include "DescriptorAllocator.h"
#include "../../logicalDevice/LogicalDevice.h"

namespace {

  // Frees descriptor sets when the frames that may still bind them have completed.
  struct DescriptorSetRelease {
    std::shared_ptr<vke::DescriptorAllocator> allocator;
    vk::DescriptorPool pool;
    std::vector<vk::DescriptorSet> sets;

    // A constructor rather than an aggregate, since make_shared can't aggregate-initialize before C++20 support for
    // parenthesized aggregate initialization (missing from the Linux CI's clang).
    DescriptorSetRelease(std::shared_ptr<vke::DescriptorAllocator> allocator,
                         const vk::DescriptorPool pool,
                         std::vector<vk::DescriptorSet> sets)
      : allocator(std::move(allocator)), pool(pool), sets(std::move(sets))
    {}

    DescriptorSetRelease(const DescriptorSetRelease&) = delete;
    DescriptorSetRelease& operator=(const DescriptorSetRelease&) = delete;

    ~DescriptorSetRelease()
    {
      allocator->free(pool, sets);
    }
  };

}

namespace vke {

  DescriptorSet::DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                               const vk::DescriptorPool descriptorPool,
                               const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings,
                               const void* allocationPNext)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createDescriptorSetLayout(layoutBindings);

    allocateDescriptorSets(descriptorPool, allocationPNext);
  }

  DescriptorSet::DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                               const vk::DescriptorPool descriptorPool,
                               const vk::DescriptorSetLayout descriptorSetLayout,
                               const void* allocationPNext)
    : m_logicalDevice(std::move(logicalDevice)), m_descriptorSetLayout(descriptorSetLayout)
  {
    allocateDescriptorSets(descriptorPool, allocationPNext);
  }

  DescriptorSet::DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                               std::shared_ptr<DescriptorAllocator> descriptorAllocator,
                               const vk::DescriptorSetLayout descriptorSetLayout,
                               const void* allocationPNext)
    : m_logicalDevice(std::move(logicalDevice)),
      m_descriptorSetLayout(descriptorSetLayout),
      m_descriptorAllocator(std::move(descriptorAllocator))
  {
    const std::vector layouts(m_logicalDevice->getMaxFramesInFlight(), m_descriptorSetLayout);

    auto allocation = m_descriptorAllocator->allocate(layouts, allocationPNext);

    m_descriptorSets = std::move(allocation.sets);
    m_allocatorPool = allocation.pool;
  }

  DescriptorSet::~DescriptorSet()
  {
    if (!m_descriptorAllocator)
    {
      return;
    }

    // A frame in flight may still bind the sets, so they are freed once it has completed.
    m_logicalDevice->retire(std::make_shared<DescriptorSetRelease>(
      std::move(m_descriptorAllocator), m_allocatorPool, std::move(m_descriptorSets)
    ));
  }

  void DescriptorSet::updateDescriptorSets(const std::function<std::vector<vk::WriteDescriptorSet>(vk::DescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const
  {
    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
    {
      std::vector<vk::WriteDescriptorSet> writeDescriptorSets = getWriteDescriptorSets(m_descriptorSets[i], i);

      m_logicalDevice->updateDescriptorSets(writeDescriptorSets);
    }
  }

  vk::DescriptorSetLayout DescriptorSet::getDescriptorSetLayout() const
  {
    return m_descriptorSetLayout;
  }

  vk::DescriptorSet DescriptorSet::getDescriptorSet(const size_t frame) const
  {
    return m_descriptorSets[frame];
  }

  void DescriptorSet::createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings)
  {
    const vk::DescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
      .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
      .pBindings = layoutBindings.data()
    };

    m_descriptorSetLayoutRAII = m_logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
    m_descriptorSetLayout = *m_descriptorSetLayoutRAII;
  }

  void DescriptorSet::allocateDescriptorSets(const vk::DescriptorPool descriptorPool,
                                             const void* allocationPNext)
  {
    const std::vector layouts(m_logicalDevice->getMaxFramesInFlight(), m_descriptorSetLayout);
    const vk::DescriptorSetAllocateInfo allocateInfo {
      .pNext = allocationPNext,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = m_logicalDevice->getMaxFramesInFlight(),
      .pSetLayouts = layouts.data()
    };

    auto raiiDescriptorSets = m_logicalDevice->allocateDescriptorSets(allocateInfo);
    m_descriptorSets.reserve(raiiDescriptorSets.size());

    for (auto& descriptorSet : raiiDescriptorSets)
    {
      m_descriptorSets.push_back(descriptorSet.release());
    }
  }

} // namespace vke
