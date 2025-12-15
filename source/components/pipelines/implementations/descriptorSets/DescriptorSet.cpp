#include "DescriptorSet.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include <vector>

namespace vke {

DescriptorSet::DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             VkDescriptorPool descriptorPool,
                             const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings)
  : m_logicalDevice(logicalDevice)
{
  createDescriptorSetLayout(layoutBindings);

  allocateDescriptorSets(descriptorPool);
}

DescriptorSet::~DescriptorSet()
{
  m_logicalDevice->destroyDescriptorSetLayout(m_descriptorSetLayout);
}

void DescriptorSet::updateDescriptorSets(const std::function<std::vector<VkWriteDescriptorSet>(VkDescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const
{
  for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = getWriteDescriptorSets(m_descriptorSets[i], i);

    m_logicalDevice->updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data());
  }
}

VkDescriptorSetLayout DescriptorSet::getDescriptorSetLayout() const
{
  return m_descriptorSetLayout;
}

VkDescriptorSet& DescriptorSet::getDescriptorSet(const size_t frame)
{
  return m_descriptorSets[frame];
}

void DescriptorSet::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings)
{
  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
    .pBindings = layoutBindings.data()
  };

  m_descriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void DescriptorSet::allocateDescriptorSets(VkDescriptorPool descriptorPool)
{
  const std::vector<VkDescriptorSetLayout> layouts(m_logicalDevice->getMaxFramesInFlight(), m_descriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = m_logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  m_descriptorSets.resize(m_logicalDevice->getMaxFramesInFlight());
  m_logicalDevice->allocateDescriptorSets(allocateInfo, m_descriptorSets.data());
}

} // namespace vke