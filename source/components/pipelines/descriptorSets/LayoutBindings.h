#ifndef VKE_LAYOUTBINDINGS_H
#define VKE_LAYOUTBINDINGS_H

#include <vector>
#include <vulkan/vulkan_raii.hpp>

constexpr uint32_t MAX_SHADOW_MAPS = 16;

namespace vke::LayoutBindings {

  constexpr vk::DescriptorSetLayoutBinding lightMetadataLayout {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR
  };

  constexpr vk::DescriptorSetLayoutBinding pointLightsLayout {
    .binding = 1,
    .descriptorType = vk::DescriptorType::eStorageBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR
  };

  constexpr vk::DescriptorSetLayoutBinding spotLightsLayout {
    .binding = 2,
    .descriptorType = vk::DescriptorType::eStorageBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR
  };

  constexpr vk::DescriptorSetLayoutBinding spotLightsSamplerLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = MAX_SHADOW_MAPS,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding pointLightsSamplerLayout {
    .binding = 5,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = MAX_SHADOW_MAPS,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding cameraLayout {
    .binding = 3,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR
  };

  constexpr vk::DescriptorSetLayoutBinding ellipticalDotsLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding noiseOptionsLayout {
    .binding = 6,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding noiseSamplerLayout {
    .binding = 7,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding curtainLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding crossesLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding chromaDepthLayout {
    .binding = 6,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding cubeMapLayout {
    .binding = 1,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding reflectUnitLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding refractUnitLayout {
    .binding = 5,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding MVPTransformLayout {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex
  };

  constexpr vk::DescriptorSetLayoutBinding bendyLayout {
    .binding = 1,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex
  };

  constexpr vk::DescriptorSetLayoutBinding textureLayout {
    .binding = 2,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> bendyLayoutBindings {
    MVPTransformLayout,
    bendyLayout,
    textureLayout
  };

  constexpr vk::DescriptorSetLayoutBinding magnifyWhirlMosaicLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  constexpr vk::DescriptorSetLayoutBinding snakeLayout {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> lightingLayoutBindings {
    lightMetadataLayout,
    pointLightsLayout,
    spotLightsLayout,
    cameraLayout,
    spotLightsSamplerLayout,
    pointLightsSamplerLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> bumpyCurtainLayoutBindings {
    curtainLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> crossesLayoutBindings {
    crossesLayout,
    chromaDepthLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> cubeMapLayoutBindings {
    cameraLayout,
    cubeMapLayout,
    noiseOptionsLayout,
    noiseSamplerLayout,
    reflectUnitLayout,
    refractUnitLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> curtainLayoutBindings {
    curtainLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> ellipticalDotsLayoutBindings {
    ellipticalDotsLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> lineLayoutBindings {
    transformLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> magnifyWhirlMosaicLayoutBindings {
    cameraLayout,
    magnifyWhirlMosaicLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> noisyEllipticalDotsLayoutBindings {
    ellipticalDotsLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> snakeLayoutBindings {
    snakeLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> texturedPlaneBindings {
    cameraLayout
  };

  constexpr vk::DescriptorSetLayoutBinding gridLayout {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> gridLayoutBindings {
    gridLayout
  };

  inline std::vector<vk::DescriptorSetLayoutBinding> pointLightShadowMapBindings {
    {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex
    }
  };

} // namespace vke::LayoutBindings

#endif //VKE_LAYOUTBINDINGS_H
