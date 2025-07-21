#ifndef LAYOUTBINDINGS_H
#define LAYOUTBINDINGS_H

#include <vector>
#include <vulkan/vulkan.h>

namespace LayoutBindings {
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

  inline std::vector<VkDescriptorSetLayoutBinding> lightingLayoutBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout
  };

  constexpr VkDescriptorSetLayoutBinding ellipticalDotsLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  inline std::vector<VkDescriptorSetLayoutBinding> ellipticalDotsLayoutBindings {
    ellipticalDotsLayout
  };

  constexpr VkDescriptorSetLayoutBinding noiseOptionsLayout {
    .binding = 6,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding noiseSamplerLayout {
    .binding = 7,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  inline std::vector<VkDescriptorSetLayoutBinding> noisyEllipticalDotsLayoutBindings {
    ellipticalDotsLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };
}

#endif //LAYOUTBINDINGS_H
