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

  constexpr VkDescriptorSetLayoutBinding ellipticalDotsLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
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

  constexpr VkDescriptorSetLayoutBinding curtainLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding crossesLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding chromaDepthLayout {
    .binding = 6,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding cubeMapLayout {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding reflectUnitLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding refractUnitLayout {
    .binding = 5,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding magnifyWhirlMosaicLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding snakeLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  inline std::vector<VkDescriptorSetLayoutBinding> lightingLayoutBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> lightingNoCameraLayoutBindings {
    lightMetadataLayout,
    lightsLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> bumpyCurtainLayoutBindings {
    curtainLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> crossesLayoutBindings {
    crossesLayout,
    chromaDepthLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> cubeMapLayoutBindings {
    cameraLayout,
    cubeMapLayout,
    noiseOptionsLayout,
    noiseSamplerLayout,
    reflectUnitLayout,
    refractUnitLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> curtainLayoutBindings {
    curtainLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> ellipticalDotsLayoutBindings {
    ellipticalDotsLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> lineLayoutBindings {
    transformLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> magnifyWhirlMosaicLayoutBindings {
    cameraLayout,
    magnifyWhirlMosaicLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> noisyEllipticalDotsLayoutBindings {
    ellipticalDotsLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> snakeLayoutBindings {
    snakeLayout
  };

  inline std::vector<VkDescriptorSetLayoutBinding> texturedPlaneBindings {
    cameraLayout
  };
}

#endif //LAYOUTBINDINGS_H
