#include "LightingDescriptorSet.h"

constexpr VkDescriptorSetLayoutBinding lightMetadataLayout {
  .binding = 2,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding lightsLayout {
  .binding = 5,
  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding cameraLayout {
  .binding = 3,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

LightingDescriptorSet::LightingDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                             VkDescriptorPool descriptorPool)
  : DescriptorSet(logicalDevice, descriptorPool)
{
  createDescriptorSet();
}

std::vector<VkDescriptorSetLayoutBinding> LightingDescriptorSet::getLayoutBindings()
{
  const std::vector<VkDescriptorSetLayoutBinding> layoutBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout
  };

  return layoutBindings;
}
