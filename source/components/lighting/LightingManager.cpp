#include <array>
#include <type_traits>
#include "LightingManager.h"
#include "lights/DirectionalLight.h"
#include "lights/Light.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "../assets/objects/Mesh.h"
#include "../assets/objects/Model.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/descriptorSets/DescriptorSet.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../pipelines/uniformBuffers/UniformBuffer.h"
#include "../renderingManager/ImageResource.h"
#include "../renderingManager/renderer3D/DrawRun.h"

#include <algorithm>

namespace {

  constexpr uint32_t MAX_SHADOW_MAPS = 16;

  // One instanced draw per run; the runs' first instances index the draw slots the shadow shaders read. The scene set is
  // bound by the caller.
  void drawShadowCasters(const std::shared_ptr<vke::CommandBuffer>& commandBuffer,
                         const std::span<const vke::DrawRun> runs)
  {
    for (const vke::DrawRun& run : runs)
    {
      run.model->draw(commandBuffer, run.firstInstance, run.instanceCount);
    }
  }

  // The same for volume surface chunks, which draw meshes
  void drawVolumeShadowCasters(const std::shared_ptr<vke::CommandBuffer>& commandBuffer,
                               const std::span<const vke::DrawRun> runs)
  {
    for (const vke::DrawRun& run : runs)
    {
      run.mesh->draw(commandBuffer, run.firstInstance, run.instanceCount);
    }
  }

  // The point/spot light storage buffers are allocated once at these capacities and never
  // resized, so the descriptor sets that reference them are written once and stay valid for
  // in-flight frames. Lights beyond the cap are ignored.
  constexpr uint32_t MAX_POINT_LIGHTS = 64;
  constexpr uint32_t MAX_SPOT_LIGHTS = 64;
  constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;

  // Each directional light that casts shadows has a map, so every light can have one
  constexpr uint32_t MAX_DIRECTIONAL_SHADOW_MAPS = MAX_DIRECTIONAL_LIGHTS;

  // m_lightUniformScratch is plain operator new storage, which is only aligned to the default new alignment.
  static_assert(alignof(vke::PointLightUniform) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ &&
                alignof(vke::SpotLightUniform) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ &&
                alignof(vke::DirectionalLightUniform) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                "light uniforms need over-aligned scratch storage");
  static_assert(std::is_trivially_copyable_v<vke::PointLightUniform> && std::is_trivially_copyable_v<vke::SpotLightUniform> &&
                std::is_trivially_copyable_v<vke::DirectionalLightUniform>);

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

  constexpr vk::DescriptorSetLayoutBinding cameraLayout {
    .binding = 3,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
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

  constexpr vk::DescriptorSetLayoutBinding directionalLightsLayout {
    .binding = 6,
    .descriptorType = vk::DescriptorType::eStorageBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR
  };

  constexpr vk::DescriptorSetLayoutBinding directionalLightsSamplerLayout {
    .binding = 7,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = MAX_DIRECTIONAL_SHADOW_MAPS,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  inline std::vector lightingLayoutBindings {
    lightMetadataLayout,
    pointLightsLayout,
    spotLightsLayout,
    cameraLayout,
    spotLightsSamplerLayout,
    pointLightsSamplerLayout,
    directionalLightsLayout,
    directionalLightsSamplerLayout
  };

}

namespace {

  struct LightMetadataUniform {
    int numPointLights;
    int numSpotLights;
    int numDirectionalLights;
  };

  using CameraUniform = glm::vec3;

}

namespace vke {

  LightingManager::LightingManager(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    createUniforms();

    createDescriptorSet();

    createShadowMapSampler();
  }

  std::shared_ptr<Light> LightingManager::createPointLight(const glm::vec3 position,
                                                           const glm::vec3 color,
                                                           const float ambient,
                                                           const float diffuse,
                                                           const float specular)
  {
    CommonLightData commonLightData {
      .position = position,
      .color = color,
      .ambient = ambient,
      .diffuse = diffuse,
      .specular = specular
    };

    auto light = std::make_shared<PointLight>(
      m_logicalDevice,
      commonLightData,
      m_commandPool
    );

    return light;
  }

  std::shared_ptr<Light> LightingManager::createSpotLight(const glm::vec3 position,
                                                          const glm::vec3 color,
                                                          const float ambient,
                                                          const float diffuse,
                                                          const float specular)
  {
    CommonLightData commonLightData {
      .position = position,
      .color = color,
      .ambient = ambient,
      .diffuse = diffuse,
      .specular = specular
    };

    auto light = std::make_shared<SpotLight>(
      m_logicalDevice,
      commonLightData,
      m_commandPool
    );

    return light;
  }

  std::shared_ptr<Light> LightingManager::createDirectionalLight(const glm::vec3 direction,
                                                                 const glm::vec3 color,
                                                                 const float ambient,
                                                                 const float diffuse,
                                                                 const float specular)
  {
    CommonLightData commonLightData {
      .position = glm::vec3(0.0f),
      .color = color,
      .ambient = ambient,
      .diffuse = diffuse,
      .specular = specular
    };

    auto light = std::make_shared<DirectionalLight>(
      m_logicalDevice,
      commonLightData,
      direction,
      m_commandPool
    );

    return light;
  }

  void LightingManager::renderLight(const std::shared_ptr<Light>& light)
  {
    const auto lightType = light->getLightType();

    if (lightType == LightType::spotLight)
    {
      m_spotLightsToRender.push_back(light);
    }
    else if (lightType == LightType::pointLight)
    {
      m_pointLightsToRender.push_back(light);
    }
    else if (lightType == LightType::directionalLight && m_directionalLightsToRender.size() < MAX_DIRECTIONAL_LIGHTS)
    {
      m_directionalLightsToRender.push_back(light);
    }
  }

  std::shared_ptr<DescriptorSet> LightingManager::getLightingDescriptorSet() const
  {
    return m_lightingDescriptorSet;
  }

  void LightingManager::clearLightsToRender()
  {
    m_pointLightsToRender.clear();
    m_spotLightsToRender.clear();
    m_directionalLightsToRender.clear();
  }

  void LightingManager::update(const uint32_t currentFrame, const glm::vec3 viewPosition)
  {
    updateUniforms(currentFrame, viewPosition);
  }

  const std::vector<std::shared_ptr<Light>>& LightingManager::getPointLightsToRender() const
  {
    return m_pointLightsToRender;
  }

  const std::vector<std::shared_ptr<Light>>& LightingManager::getSpotLightsToRender() const
  {
    return m_spotLightsToRender;
  }

  const std::vector<std::shared_ptr<Light>>& LightingManager::getDirectionalLightsToRender() const
  {
    return m_directionalLightsToRender;
  }

  std::shared_ptr<PointLight> LightingManager::asShadowCastingPointLight(const std::shared_ptr<Light>& light)
  {
    auto pointLight = std::dynamic_pointer_cast<PointLight>(light);

    return pointLight && pointLight->castsShadows() ? pointLight : nullptr;
  }

  std::shared_ptr<SpotLight> LightingManager::asShadowCastingSpotLight(const std::shared_ptr<Light>& light)
  {
    auto spotLight = std::dynamic_pointer_cast<SpotLight>(light);

    return spotLight && spotLight->castsShadows() ? spotLight : nullptr;
  }

  std::shared_ptr<DirectionalLight> LightingManager::asShadowCastingDirectionalLight(const std::shared_ptr<Light>& light)
  {
    auto directionalLight = std::dynamic_pointer_cast<DirectionalLight>(light);

    return directionalLight && directionalLight->castsShadows() ? directionalLight : nullptr;
  }

  void LightingManager::renderShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                         const std::shared_ptr<PipelineManager>& pipelineManager,
                                         const ShadowDrawLists& lists,
                                         const vk::DescriptorSet sceneDescriptorSet,
                                         const uint32_t currentFrame) const
  {
    renderPointLightShadowMaps(commandBuffer, pipelineManager, lists.pointLightFaceRuns, lists.volumePointLightFaceRuns,
                               sceneDescriptorSet);

    renderSpotLightShadowMaps(commandBuffer, pipelineManager, lists.spotLightRuns, lists.volumeSpotLightRuns,
                              sceneDescriptorSet, currentFrame);

    renderDirectionalShadowMaps(commandBuffer, pipelineManager, lists.directionalLightRuns,
                                lists.volumeDirectionalLightRuns, sceneDescriptorSet);
  }

  void LightingManager::createUniforms()
  {
    m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

    m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(PointLightUniform) * MAX_POINT_LIGHTS);

    m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SpotLightUniform) * MAX_SPOT_LIGHTS);

    m_directionalLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice,
                                                                 sizeof(DirectionalLightUniform) * MAX_DIRECTIONAL_LIGHTS);

    m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));
  }

  void LightingManager::createDescriptorSet()
  {
    m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, getDescriptorPool(), lightingLayoutBindings);
    m_lightingDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector descriptorWrites{{
        m_lightMetadataUniform->getDescriptorSet(0, descriptorSet, frame),
        m_pointLightsUniform->getDescriptorSet(1, descriptorSet, frame),
        m_spotLightsUniform->getDescriptorSet(2, descriptorSet, frame),
        m_cameraUniform->getDescriptorSet(3, descriptorSet, frame),
        m_directionalLightsUniform->getDescriptorSet(6, descriptorSet, frame)
      }};

      descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
      descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageBuffer;
      descriptorWrites[4].descriptorType = vk::DescriptorType::eStorageBuffer;

      return descriptorWrites;
    });
  }

  void LightingManager::updateUniforms(const uint32_t currentFrame,
                                       const glm::vec3 viewPosition)
  {
    const CameraUniform cameraUBO = viewPosition;

    m_cameraUniform->update(currentFrame, &cameraUBO);

    updateLightMetadataUniform();

    updatePointLightUniforms(currentFrame);

    updateSpotLightUniforms(currentFrame);

    updateDirectionalLightShadowMaps(currentFrame);
  }

  void LightingManager::updatePointLightUniforms(const uint32_t currentFrame)
  {
    if (m_pointLightsToRender.empty())
    {
      return;
    }

    const size_t count = std::min<size_t>(m_pointLightsToRender.size(), MAX_POINT_LIGHTS);

    // Sized to the buffer's fixed capacity: UniformBuffer::update() copies the whole buffer, so
    // the source must cover it. Unused entries stay zeroed and are ignored via the light count.
    m_lightUniformScratch.assign(sizeof(PointLightUniform) * MAX_POINT_LIGHTS, std::byte{0});
    auto* lightUniforms = reinterpret_cast<PointLightUniform*>(m_lightUniformScratch.data());
    // The maps are written in this order by updatePointLightShadowMaps, so a caster's index is its place among them
    int32_t shadowMapCount = 0;
    for (size_t i = 0; i < count; i++)
    {
      lightUniforms[i] = std::get<PointLightUniform>(m_pointLightsToRender[i]->getUniform());

      if (m_pointLightsToRender[i]->castsShadows() && shadowMapCount < static_cast<int32_t>(MAX_SHADOW_MAPS))
      {
        lightUniforms[i].shadowMapIndex = shadowMapCount++;
      }
    }

    m_pointLightsUniform->update(currentFrame, lightUniforms);

    updatePointLightShadowMaps(currentFrame);
  }

  void LightingManager::updatePointLightShadowMaps(const uint32_t currentFrame) const
  {
    std::array<vk::DescriptorImageInfo, MAX_SHADOW_MAPS> imageInfos;
    size_t imageInfoCount = 0;

    for (auto& light : m_pointLightsToRender)
    {
      if (!light->castsShadows())
      {
        continue;
      }

      imageInfos[imageInfoCount++] = {
        m_pointShadowMapSampler,
        light->getShadowMapDepthImageResource()->getImageView(),
        vk::ImageLayout::eDepthStencilReadOnlyOptimal
      };

      if (imageInfoCount >= MAX_SHADOW_MAPS)
      {
        break;
      }
    }

    if (imageInfoCount == 0)
    {
      return;
    }

    const vk::WriteDescriptorSet samplerWrite {
      .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
      .dstBinding = 5,
      .dstArrayElement = 0,
      .descriptorCount = static_cast<uint32_t>(imageInfoCount),
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = imageInfos.data(),
    };

    m_logicalDevice->updateDescriptorSets({ samplerWrite });
  }

  void LightingManager::updateSpotLightUniforms(const uint32_t currentFrame)
  {
    if (m_spotLightsToRender.empty())
    {
      return;
    }

    const size_t count = std::min<size_t>(m_spotLightsToRender.size(), MAX_SPOT_LIGHTS);

    // Sized to the buffer's fixed capacity: UniformBuffer::update() copies the whole buffer, so
    // the source must cover it. Unused entries stay zeroed and are ignored via the light count.
    m_lightUniformScratch.assign(sizeof(SpotLightUniform) * MAX_SPOT_LIGHTS, std::byte{0});
    auto* lightUniforms = reinterpret_cast<SpotLightUniform*>(m_lightUniformScratch.data());
    // The maps are written in this order by updateSpotLightShadowMaps, so a caster's index is its place among them
    int32_t shadowMapCount = 0;
    for (size_t i = 0; i < count; i++)
    {
      lightUniforms[i] = std::get<SpotLightUniform>(m_spotLightsToRender[i]->getUniform());

      if (m_spotLightsToRender[i]->castsShadows() && shadowMapCount < static_cast<int32_t>(MAX_SHADOW_MAPS))
      {
        lightUniforms[i].shadowMapIndex = shadowMapCount++;
      }
    }

    m_spotLightsUniform->update(currentFrame, lightUniforms);

    updateSpotLightShadowMaps(currentFrame);
  }

  void LightingManager::updateSpotLightShadowMaps(const uint32_t currentFrame) const
  {
    std::array<vk::DescriptorImageInfo, MAX_SHADOW_MAPS> imageInfos;
    size_t imageInfoCount = 0;

    for (auto& light : m_spotLightsToRender)
    {
      if (!light->castsShadows())
      {
        continue;
      }

      imageInfos[imageInfoCount++] = {
        m_shadowMapSampler,
        light->getShadowMapDepthImageResource()->getImageView(),
        vk::ImageLayout::eDepthStencilReadOnlyOptimal
      };

      if (imageInfoCount >= MAX_SHADOW_MAPS)
      {
        break;
      }
    }

    if (imageInfoCount == 0)
    {
      return;
    }

    const vk::WriteDescriptorSet samplerWrite {
      .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
      .dstBinding = 4,
      .dstArrayElement = 0,
      .descriptorCount = static_cast<uint32_t>(imageInfoCount),
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = imageInfos.data(),
    };

    m_logicalDevice->updateDescriptorSets({ samplerWrite });
  }

  void LightingManager::writeDirectionalLights(const uint32_t currentFrame)
  {
    if (m_directionalLightsToRender.empty())
    {
      return;
    }

    // renderLight already keeps at most MAX_DIRECTIONAL_LIGHTS
    const size_t count = std::min<size_t>(m_directionalLightsToRender.size(), MAX_DIRECTIONAL_LIGHTS);

    // Sized to the buffer's fixed capacity, as for the spot lights
    m_lightUniformScratch.assign(sizeof(DirectionalLightUniform) * MAX_DIRECTIONAL_LIGHTS, std::byte{0});
    auto* lightUniforms = reinterpret_cast<DirectionalLightUniform*>(m_lightUniformScratch.data());
    // The maps are written in this order by updateDirectionalLightShadowMaps (in update)
    int32_t shadowMapCount = 0;
    for (size_t i = 0; i < count; i++)
    {
      lightUniforms[i] = std::get<DirectionalLightUniform>(m_directionalLightsToRender[i]->getUniform());

      if (m_directionalLightsToRender[i]->castsShadows() &&
          shadowMapCount < static_cast<int32_t>(MAX_DIRECTIONAL_SHADOW_MAPS))
      {
        lightUniforms[i].shadowMapIndex = shadowMapCount++;
      }
    }

    m_directionalLightsUniform->update(currentFrame, lightUniforms);
  }

  void LightingManager::updateDirectionalLightShadowMaps(const uint32_t currentFrame) const
  {
    std::array<vk::DescriptorImageInfo, MAX_DIRECTIONAL_SHADOW_MAPS> imageInfos;
    size_t imageInfoCount = 0;

    for (auto& light : m_directionalLightsToRender)
    {
      if (!light->castsShadows())
      {
        continue;
      }

      imageInfos[imageInfoCount++] = {
        m_shadowMapSampler,
        light->getShadowMapDepthImageResource()->getImageView(),
        vk::ImageLayout::eDepthStencilReadOnlyOptimal
      };

      if (imageInfoCount >= MAX_DIRECTIONAL_SHADOW_MAPS)
      {
        break;
      }
    }

    if (imageInfoCount == 0)
    {
      return;
    }

    const vk::WriteDescriptorSet samplerWrite {
      .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
      .dstBinding = 7,
      .dstArrayElement = 0,
      .descriptorCount = static_cast<uint32_t>(imageInfoCount),
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = imageInfos.data(),
    };

    m_logicalDevice->updateDescriptorSets({ samplerWrite });
  }

  void LightingManager::createShadowMapSampler()
  {
    constexpr vk::SamplerCreateInfo samplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eNearest,
      .addressModeU = vk::SamplerAddressMode::eClampToBorder,
      .addressModeV = vk::SamplerAddressMode::eClampToBorder,
      .addressModeW = vk::SamplerAddressMode::eClampToBorder,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::False,
      .compareEnable = vk::True,
      .compareOp = vk::CompareOp::eLessOrEqual,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = vk::BorderColor::eFloatOpaqueWhite,
      .unnormalizedCoordinates = vk::False,
    };

    m_shadowMapSampler = m_logicalDevice->createSampler(samplerInfo);

    vk::SamplerCreateInfo pointSamplerInfo = samplerInfo;
    pointSamplerInfo.compareOp = vk::CompareOp::eGreaterOrEqual;

    m_pointShadowMapSampler = m_logicalDevice->createSampler(pointSamplerInfo);
  }

  void LightingManager::renderPointLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   const std::shared_ptr<PipelineManager>& pipelineManager,
                                                   const std::span<const std::vector<DrawRun>> faceRuns,
                                                   const std::span<const std::vector<DrawRun>> volumeFaceRuns,
                                                   const vk::DescriptorSet sceneDescriptorSet) const
  {
    size_t shadowCaster = 0;

    for (auto& light : m_pointLightsToRender)
    {
      const auto pointLight = asShadowCastingPointLight(light);
      if (!pointLight)
      {
        continue;
      }

      // Renderer3D builds six lists per shadow-casting light, in this order and in face order
      const size_t firstList = shadowCaster++ * 6;

      const auto shadowExtent = light->getShadowMapExtent();
      const auto viewProjections = pointLight->getLightViewProjectionMatrices();
      const auto& shadowMap = light->getShadowMapDepthImageResource();

      transitionShadowMapForRendering(commandBuffer, light);

      for (uint32_t face = 0; face < 6; ++face)
      {
        const size_t list = firstList + face;
        const std::span<const DrawRun> runs = list < faceRuns.size()
                                              ? std::span<const DrawRun>(faceRuns[list])
                                              : std::span<const DrawRun>();
        const std::span<const DrawRun> volumeRuns = list < volumeFaceRuns.size()
                                                    ? std::span<const DrawRun>(volumeFaceRuns[list])
                                                    : std::span<const DrawRun>();

        // Every face is begun, so that an empty one is still cleared
        beginShadowRendering(commandBuffer, shadowMap->getFaceImageView(face), shadowExtent, 0.0f);

        if (!runs.empty())
        {
          constexpr auto pipeline = PipelineType::pointLightShadowMap;

          pipelineManager->bindGraphicsPipeline(commandBuffer, pipeline);

          pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(commandBuffer, pipeline,
                                                                    vk::ShaderStageFlagBits::eVertex, 0,
                                                                    viewProjections[face]);

          pipelineManager->bindGraphicsPipelineDescriptorSet(commandBuffer, pipeline, sceneDescriptorSet, 0);

          drawShadowCasters(commandBuffer, runs);
        }

        if (!volumeRuns.empty())
        {
          constexpr auto volumePipeline = PipelineType::volumeSurfacePointLightShadowMap;

          pipelineManager->bindGraphicsPipeline(commandBuffer, volumePipeline);

          pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(commandBuffer, volumePipeline,
                                                                    vk::ShaderStageFlagBits::eVertex, 0,
                                                                    viewProjections[face]);

          // Rebound, since the pipeline layouts differ
          pipelineManager->bindGraphicsPipelineDescriptorSet(commandBuffer, volumePipeline, sceneDescriptorSet, 0);

          drawVolumeShadowCasters(commandBuffer, volumeRuns);
        }

        commandBuffer->endRendering();
      }

      transitionShadowMapForSampling(commandBuffer, light);
    }
  }

  void LightingManager::renderSpotLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const std::shared_ptr<PipelineManager>& pipelineManager,
                                                  const std::span<const std::vector<DrawRun>> spotLightRuns,
                                                  const std::span<const std::vector<DrawRun>> volumeSpotLightRuns,
                                                  const vk::DescriptorSet sceneDescriptorSet,
                                                  const uint32_t currentFrame) const
  {
    size_t shadowCaster = 0;

    for (auto& light : m_spotLightsToRender)
    {
      const auto spotLight = asShadowCastingSpotLight(light);
      if (!spotLight)
      {
        continue;
      }

      // Renderer3D builds one list per shadow-casting light, in this order
      const std::span<const DrawRun> runs = shadowCaster < spotLightRuns.size()
                                            ? std::span<const DrawRun>(spotLightRuns[shadowCaster])
                                            : std::span<const DrawRun>();
      const std::span<const DrawRun> volumeRuns = shadowCaster < volumeSpotLightRuns.size()
                                                  ? std::span<const DrawRun>(volumeSpotLightRuns[shadowCaster])
                                                  : std::span<const DrawRun>();
      ++shadowCaster;

      const auto shadowExtent = light->getShadowMapExtent();

      transitionShadowMapForRendering(commandBuffer, light);

      beginShadowRendering(commandBuffer, light->getShadowMapDepthImageResource()->getImageView(), shadowExtent, 1.0f);

      RenderInfo shadowRenderInfo = {
        .commandBuffer = commandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = light->getPosition(),
        .viewMatrix = spotLight->getLightViewProjectionMatrix(),
        .extent = shadowExtent
      };

      pipelineManager->bindGraphicsPipeline(shadowRenderInfo.commandBuffer, PipelineType::shadow);

      pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(
        shadowRenderInfo.commandBuffer,
        PipelineType::shadow,
        vk::ShaderStageFlagBits::eVertex,
        0,
        shadowRenderInfo.viewMatrix
      );

      pipelineManager->bindGraphicsPipelineDescriptorSet(
        shadowRenderInfo.commandBuffer,
        PipelineType::shadow,
        sceneDescriptorSet,
        0
      );

      drawShadowCasters(shadowRenderInfo.commandBuffer, runs);

      if (!volumeRuns.empty())
      {
        constexpr auto volumePipeline = PipelineType::volumeSurfaceShadow;

        pipelineManager->bindGraphicsPipeline(shadowRenderInfo.commandBuffer, volumePipeline);

        pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(
          shadowRenderInfo.commandBuffer,
          volumePipeline,
          vk::ShaderStageFlagBits::eVertex,
          0,
          shadowRenderInfo.viewMatrix
        );

        // Rebound, since the pipeline layouts differ
        pipelineManager->bindGraphicsPipelineDescriptorSet(shadowRenderInfo.commandBuffer, volumePipeline, sceneDescriptorSet, 0);

        drawVolumeShadowCasters(shadowRenderInfo.commandBuffer, volumeRuns);
      }

      commandBuffer->endRendering();

      transitionShadowMapForSampling(commandBuffer, light);
    }
  }

  void LightingManager::renderDirectionalShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                                    const std::span<const std::vector<DrawRun>> directionalLightRuns,
                                                    const std::span<const std::vector<DrawRun>> volumeDirectionalLightRuns,
                                                    const vk::DescriptorSet sceneDescriptorSet) const
  {
    size_t shadowCaster = 0;

    for (auto& light : m_directionalLightsToRender)
    {
      const auto directionalLight = asShadowCastingDirectionalLight(light);
      if (!directionalLight)
      {
        continue;
      }

      // Renderer3D builds s_maxShadowCascades lists per shadow-casting light, in this order and in cascade order
      const size_t firstList = shadowCaster++ * s_maxShadowCascades;

      transitionShadowMapForRendering(commandBuffer, light);

      const auto& shadowMap = light->getShadowMapDepthImageResource();

      for (uint32_t cascade = 0; cascade < directionalLight->getFittedCascadeCount(); ++cascade)
      {
        const size_t list = firstList + cascade;
        const std::span<const DrawRun> runs = list < directionalLightRuns.size()
                                              ? std::span<const DrawRun>(directionalLightRuns[list])
                                              : std::span<const DrawRun>();
        const std::span<const DrawRun> volumeRuns = list < volumeDirectionalLightRuns.size()
                                                    ? std::span<const DrawRun>(volumeDirectionalLightRuns[list])
                                                    : std::span<const DrawRun>();

        const glm::mat4& viewProjection = directionalLight->getCascadeViewProjection(cascade);

        // Every fitted cascade is begun, so that an empty one is still cleared
        beginShadowRendering(commandBuffer, shadowMap->getLayerImageView(cascade), light->getShadowMapExtent(), 1.0f);

        if (!runs.empty())
        {
          constexpr auto pipeline = PipelineType::shadow;

          pipelineManager->bindGraphicsPipeline(commandBuffer, pipeline);

          pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(commandBuffer, pipeline,
                                                                    vk::ShaderStageFlagBits::eVertex, 0, viewProjection);

          pipelineManager->bindGraphicsPipelineDescriptorSet(commandBuffer, pipeline, sceneDescriptorSet, 0);

          drawShadowCasters(commandBuffer, runs);
        }

        if (!volumeRuns.empty())
        {
          constexpr auto volumePipeline = PipelineType::volumeSurfaceShadow;

          pipelineManager->bindGraphicsPipeline(commandBuffer, volumePipeline);

          pipelineManager->pushGraphicsPipelineConstants<glm::mat4>(commandBuffer, volumePipeline,
                                                                    vk::ShaderStageFlagBits::eVertex, 0, viewProjection);

          // Rebound, since the pipeline layouts differ
          pipelineManager->bindGraphicsPipelineDescriptorSet(commandBuffer, volumePipeline, sceneDescriptorSet, 0);

          drawVolumeShadowCasters(commandBuffer, volumeRuns);
        }

        commandBuffer->endRendering();
      }

      transitionShadowMapForSampling(commandBuffer, light);
    }
  }

  void LightingManager::createCommandPool()
  {
    const vk::CommandPoolCreateInfo poolInfo {
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void LightingManager::createDescriptorPool()
  {
    const std::array<vk::DescriptorPoolSize, 3> poolSizes {{
      {vk::DescriptorType::eUniformBuffer, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize},
      {vk::DescriptorType::eCombinedImageSampler,
       m_logicalDevice->getMaxFramesInFlight() * (MAX_SHADOW_MAPS * 2 + MAX_DIRECTIONAL_SHADOW_MAPS)},
      {vk::DescriptorType::eStorageBuffer, m_logicalDevice->getMaxFramesInFlight() * 20}
    }};

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPools.push_back(m_logicalDevice->createDescriptorPool(poolCreateInfo));
  }

  vk::DescriptorPool LightingManager::getDescriptorPool()
  {
    m_currentDescriptorPoolSize++;

    if (m_currentDescriptorPoolSize > m_descriptorPoolSize)
    {
      m_currentDescriptorPoolSize = 1;
      createDescriptorPool();
    }

    return m_descriptorPools.back();
  }

  void LightingManager::updateLightMetadataUniform()
  {
    const int numPointLights = static_cast<int>(std::min<size_t>(m_pointLightsToRender.size(), MAX_POINT_LIGHTS));
    const int numSpotLights = static_cast<int>(std::min<size_t>(m_spotLightsToRender.size(), MAX_SPOT_LIGHTS));

    const int numDirectionalLights = static_cast<int>(
      std::min<size_t>(m_directionalLightsToRender.size(), MAX_DIRECTIONAL_LIGHTS));

    if (m_prevNumPointLights == numPointLights &&
        m_prevNumSpotLights == numSpotLights &&
        m_prevNumDirectionalLights == numDirectionalLights)
    {
      return;
    }

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = numPointLights,
      .numSpotLights = numSpotLights,
      .numDirectionalLights = numDirectionalLights
    };

    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); ++i)
    {
      m_lightMetadataUniform->update(i, &lightMetadataUBO);
    }

    m_prevNumPointLights = numPointLights;
    m_prevNumSpotLights = numSpotLights;
    m_prevNumDirectionalLights = numDirectionalLights;
  }

  void LightingManager::transitionShadowMapForRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                        const std::shared_ptr<Light>& light)
  {
    // The shadow map was last sampled as DEPTH_STENCIL_READ_ONLY_OPTIMAL (or is in its initial
    // layout). Every layer is cleared on load, so discard the old contents.
    const vk::ImageMemoryBarrier2 shadowMapBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
      .srcAccessMask = vk::AccessFlagBits2::eNone,
      .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                      vk::PipelineStageFlagBits2::eLateFragmentTests,
      .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                       vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = light->getShadowMapDepthImageResource()->getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eDepth,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &shadowMapBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void LightingManager::beginShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const vk::ImageView depthView,
                                             const vk::Extent2D extent,
                                             const float clearDepth)
  {
    const vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = depthView,
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearDepthStencilValue{ .depth = clearDepth, .stencil = 0 }
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);

    const vk::Viewport viewport {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    commandBuffer->setViewport(viewport);

    commandBuffer->setScissor(vk::Rect2D{{0, 0}, extent});
  }

  void LightingManager::transitionShadowMapForSampling(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                       const std::shared_ptr<Light>& light)
  {
    // Hand the shadow map over to the lighting shaders, which sample it with a descriptor
    // declaring DEPTH_STENCIL_READ_ONLY_OPTIMAL.
    const vk::ImageMemoryBarrier2 shadowMapBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                      vk::PipelineStageFlagBits2::eLateFragmentTests,
      .srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
      .oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = light->getShadowMapDepthImageResource()->getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eDepth,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &shadowMapBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }
} // namespace vke