#include "DescriptorSet.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <vector>

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
    const std::vector<vk::DescriptorSetLayout> layouts(m_logicalDevice->getMaxFramesInFlight(), m_descriptorSetLayout);
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
