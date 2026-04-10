#include "LightingManager.h"
#include "lights/Light.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "../assets/objects/RenderObject.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/descriptorSets/DescriptorSet.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../pipelines/uniformBuffers/UniformBuffer.h"
#include "../renderingManager/ImageResource.h"
#include "../renderingManager/RenderTarget.h"

namespace {

  constexpr uint32_t MAX_SHADOW_MAPS = 16;

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

  inline std::vector lightingLayoutBindings {
    lightMetadataLayout,
    pointLightsLayout,
    spotLightsLayout,
    cameraLayout,
    spotLightsSamplerLayout,
    pointLightsSamplerLayout
  };

  inline std::vector pointLightShadowMapBindings {
    vk::DescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex
    }
  };

}

namespace {

  struct LightMetadataUniform {
    int numPointLights;
    int numSpotLights;
  };

  using CameraUniform = glm::vec3;

}

namespace vke {

  LightingManager::LightingManager(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    createPointLightDescriptorSetLayout();

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
      m_commandPool,
      getDescriptorPool(),
      m_pointLightDescriptorSetLayout
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
  }

  std::shared_ptr<DescriptorSet> LightingManager::getLightingDescriptorSet() const
  {
    return m_lightingDescriptorSet;
  }

  void LightingManager::clearLightsToRender()
  {
    m_pointLightsToRender.clear();
    m_spotLightsToRender.clear();
  }

  void LightingManager::update(const uint32_t currentFrame, const glm::vec3 viewPosition)
  {
    updateUniforms(currentFrame, viewPosition);
  }

  void LightingManager::renderShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                         const std::shared_ptr<PipelineManager>& pipelineManager,
                                         const std::vector<std::shared_ptr<RenderObject>>* objects,
                                         const uint32_t currentFrame) const
  {
    renderPointLightShadowMaps(commandBuffer, pipelineManager, objects, currentFrame);

    renderSpotLightShadowMaps(commandBuffer, pipelineManager, objects, currentFrame);
  }

  vk::DescriptorSetLayout LightingManager::getPointLightDescriptorSetLayout() const
  {
    return m_pointLightDescriptorSetLayout;
  }

  void LightingManager::createUniforms()
  {
    m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

    m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(PointLightUniform));

    m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SpotLightUniform));

    m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));
  }

  void LightingManager::createDescriptorSet()
  {
    m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, getDescriptorPool(), lightingLayoutBindings);
    m_lightingDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<vk::WriteDescriptorSet> descriptorWrites{{
        m_lightMetadataUniform->getDescriptorSet(0, descriptorSet, frame),
        m_pointLightsUniform->getDescriptorSet(1, descriptorSet, frame),
        m_spotLightsUniform->getDescriptorSet(2, descriptorSet, frame),
        m_cameraUniform->getDescriptorSet(3, descriptorSet, frame)
      }};

      descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
      descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageBuffer;

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
  }

  void LightingManager::updatePointLightUniforms(const uint32_t currentFrame)
  {
    if (m_prevNumPointLights != m_pointLightsToRender.size())
    {
      if (m_pointLightsToRender.empty())
      {
        m_prevNumPointLights = 0;

        return;
      }

      m_logicalDevice->waitIdle();

      m_pointLightsUniform.reset();

      auto lightsUniformBufferSize = sizeof(PointLightUniform) * m_pointLightsToRender.size();

      m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

      m_lightingDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
      {
        std::vector<vk::WriteDescriptorSet> descriptorWrites{{
          m_pointLightsUniform->getDescriptorSet(1, descriptorSet, frame)
        }};

        descriptorWrites[0].descriptorType = vk::DescriptorType::eStorageBuffer;

        return descriptorWrites;
      });

      m_prevNumPointLights = static_cast<int>(m_pointLightsToRender.size());
    }

    if (m_pointLightsToRender.empty())
    {
      return;
    }

    std::vector<PointLightUniform> lightUniforms;
    lightUniforms.resize(m_pointLightsToRender.size());
    for (int i = 0; i < m_pointLightsToRender.size(); i++)
    {
      lightUniforms[i] = std::get<PointLightUniform>(m_pointLightsToRender[i]->getUniform());
    }

    m_pointLightsUniform->update(currentFrame, lightUniforms.data());

    updatePointLightShadowMaps(currentFrame);
  }

  void LightingManager::updatePointLightShadowMaps(const uint32_t currentFrame) const
  {
    std::vector<vk::DescriptorImageInfo> imageInfos;

    for (auto& light : m_pointLightsToRender)
    {
      if (!light->castsShadows())
      {
        continue;
      }

      imageInfos.push_back({
        m_shadowMapSampler,
        light->getShadowMapDepthImageResource()->getImageView(),
        vk::ImageLayout::eDepthStencilReadOnlyOptimal
      });

      if (imageInfos.size() >= MAX_SHADOW_MAPS)
      {
        break;
      }
    }

    if (imageInfos.empty())
    {
      return;
    }

    const vk::WriteDescriptorSet samplerWrite {
      .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
      .dstBinding = 5,
      .dstArrayElement = 0,
      .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = imageInfos.data(),
    };

    m_logicalDevice->updateDescriptorSets({ samplerWrite });
  }

  void LightingManager::updateSpotLightUniforms(const uint32_t currentFrame)
  {
    if (m_prevNumSpotLights != m_spotLightsToRender.size())
    {
      if (m_spotLightsToRender.empty())
      {
        m_prevNumSpotLights = 0;

        return;
      }

      m_logicalDevice->waitIdle();

      m_spotLightsUniform.reset();

      auto lightsUniformBufferSize = sizeof(SpotLightUniform) * m_spotLightsToRender.size();

      m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

      m_lightingDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
      {
        std::vector<vk::WriteDescriptorSet> descriptorWrites{{
          m_spotLightsUniform->getDescriptorSet(2, descriptorSet, frame)
        }};

        descriptorWrites[0].descriptorType = vk::DescriptorType::eStorageBuffer;

        return descriptorWrites;
      });

      m_prevNumSpotLights = static_cast<int>(m_spotLightsToRender.size());
    }

    if (m_spotLightsToRender.empty())
    {
      return;
    }

    std::vector<SpotLightUniform> lightUniforms;
    lightUniforms.resize(m_spotLightsToRender.size());
    for (int i = 0; i < m_spotLightsToRender.size(); i++)
    {
      lightUniforms[i] = std::get<SpotLightUniform>(m_spotLightsToRender[i]->getUniform());
    }

    m_spotLightsUniform->update(currentFrame, lightUniforms.data());

    updateSpotLightShadowMaps(currentFrame);
  }

  void LightingManager::updateSpotLightShadowMaps(const uint32_t currentFrame) const
  {
    std::vector<vk::DescriptorImageInfo> imageInfos;

    for (auto& light : m_spotLightsToRender)
    {
      if (!light->castsShadows())
      {
        continue;
      }

      imageInfos.push_back({
        m_shadowMapSampler,
        light->getShadowMapDepthImageResource()->getImageView(),
        vk::ImageLayout::eDepthStencilReadOnlyOptimal
      });

      if (imageInfos.size() >= MAX_SHADOW_MAPS)
      {
        break;
      }
    }

    if (imageInfos.empty())
    {
      return;
    }

    const vk::WriteDescriptorSet samplerWrite {
      .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
      .dstBinding = 4,
      .dstArrayElement = 0,
      .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
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
  }

  void LightingManager::renderPointLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   const std::shared_ptr<PipelineManager>& pipelineManager,
                                                   const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                   const uint32_t currentFrame) const
  {
    for (auto& light : m_pointLightsToRender)
    {
      const auto pointLight = std::dynamic_pointer_cast<PointLight>(light);
      if (!pointLight->castsShadows())
      {
        continue;
      }

      const auto shadowExtent = light->getShadowMapExtent();

      beginShadowRendering(commandBuffer, light);

      vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(shadowExtent.width),
        .height = static_cast<float>(shadowExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      commandBuffer->setViewport(viewport);

      vk::Rect2D scissor = {{0, 0}, shadowExtent};
      commandBuffer->setScissor(scissor);

      RenderInfo shadowRenderInfo = {
        .commandBuffer = commandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = pointLight->getPosition(),
        .viewMatrix = glm::mat4(1.0),
        .extent = shadowExtent
      };

      pipelineManager->bindGraphicsPipeline(shadowRenderInfo.commandBuffer, PipelineType::pointLightShadowMap);

      pipelineManager->pushGraphicsPipelineConstants<glm::vec3>(
        shadowRenderInfo.commandBuffer,
        PipelineType::pointLightShadowMap,
        vk::ShaderStageFlagBits::eFragment,
        0,
        shadowRenderInfo.viewPosition
      );

      pointLight->updateUniform(shadowRenderInfo.currentFrame);

      pipelineManager->bindGraphicsPipelineDescriptorSet(
        shadowRenderInfo.commandBuffer,
        PipelineType::pointLightShadowMap,
        pointLight->getDescriptorSet(currentFrame),
        1
      );

      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(shadowRenderInfo.currentFrame, {1.0}, {1.0});

        pipelineManager->bindGraphicsPipelineDescriptorSet(
          shadowRenderInfo.commandBuffer,
          PipelineType::pointLightShadowMap,
          object->getDescriptorSet(currentFrame),
          0
        );

        object->draw(shadowRenderInfo.commandBuffer);
      }

      commandBuffer->endRendering();
    }
  }

  void LightingManager::renderSpotLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const std::shared_ptr<PipelineManager>& pipelineManager,
                                                  const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                  const uint32_t currentFrame) const
  {
    for (auto& light : m_spotLightsToRender)
    {
      const auto spotLight = std::dynamic_pointer_cast<SpotLight>(light);
      if (!spotLight->castsShadows())
      {
        continue;
      }

      const auto shadowExtent = light->getShadowMapExtent();

      beginShadowRendering(commandBuffer, light);

      vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(shadowExtent.width),
        .height = static_cast<float>(shadowExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      commandBuffer->setViewport(viewport);

      vk::Rect2D scissor = {{0, 0}, shadowExtent};
      commandBuffer->setScissor(scissor);

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

      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(shadowRenderInfo.currentFrame, {1.0}, {1.0});

        pipelineManager->bindGraphicsPipelineDescriptorSet(
          shadowRenderInfo.commandBuffer,
          PipelineType::shadow,
          object->getDescriptorSet(currentFrame),
          0
        );

        object->draw(shadowRenderInfo.commandBuffer);
      }

      commandBuffer->endRendering();
    }
  }

  void LightingManager::createPointLightDescriptorSetLayout()
  {
    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .bindingCount = static_cast<uint32_t>(pointLightShadowMapBindings.size()),
      .pBindings = pointLightShadowMapBindings.data()
    };

    m_pointLightDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
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
      {vk::DescriptorType::eCombinedImageSampler, m_logicalDevice->getMaxFramesInFlight() * MAX_SHADOW_MAPS * 2},
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

  void LightingManager::updateLightMetadataUniform() const
  {
    if (m_prevNumPointLights == m_pointLightsToRender.size() &&
        m_prevNumSpotLights == m_spotLightsToRender.size())
    {
      return;
    }

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = static_cast<int>(m_pointLightsToRender.size()),
      .numSpotLights = static_cast<int>(m_spotLightsToRender.size())
    };

    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); ++i)
    {
      m_lightMetadataUniform->update(i, &lightMetadataUBO);
    }
  }

  void LightingManager::beginShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const std::shared_ptr<Light>& light)
  {
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = light->getShadowMapDepthImageResource()->getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearDepth
    };

    constexpr uint32_t kCubemapFacesMask = 0x3Fu;
    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = light->getShadowMapExtent(),
      },
      .layerCount = 1,
      .viewMask = light->getLightType() == LightType::pointLight ? kCubemapFacesMask : 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }
} // namespace vke