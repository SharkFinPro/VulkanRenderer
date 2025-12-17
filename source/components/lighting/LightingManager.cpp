#include "LightingManager.h"
#include "lights/Light.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "../../components/logicalDevice/LogicalDevice.h"
#include "../pipelines/implementations/common/Uniforms.h"
#include "../pipelines/implementations/renderObject/ShadowPipeline.h"
#include "../pipelines/descriptorSets/DescriptorSet.h"
#include "../pipelines/descriptorSets/LayoutBindings.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../pipelines/uniformBuffers/UniformBuffer.h"
#include "../renderingManager/Renderer.h"
#include "../commandBuffer/CommandBuffer.h"

namespace vke {

LightingManager::LightingManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 VkDescriptorPool descriptorPool,
                                 VkCommandPool commandPool)
  : m_logicalDevice(logicalDevice), m_commandPool(commandPool)
{
  createUniforms();

  createDescriptorSet(descriptorPool);

  createShadowMapSampler();
}

LightingManager::~LightingManager()
{
  destroyShadowMapSampler();
}

std::shared_ptr<Light> LightingManager::createPointLight(glm::vec3 position,
                                                         glm::vec3 color,
                                                         float ambient,
                                                         float diffuse,
                                                         float specular = 1.0f)
{
  auto light = std::make_shared<PointLight>(m_logicalDevice, position, color, ambient, diffuse, specular);

  m_lights.push_back(light);

  return light;
}

  std::shared_ptr<Light> LightingManager::createSpotLight(glm::vec3 position,
                                                          glm::vec3 color,
                                                          float ambient,
                                                          float diffuse,
                                                          float specular = 1.0f)
{
  auto light = std::make_shared<SpotLight>(m_logicalDevice, position, color, ambient, diffuse, specular, m_commandPool);

  m_lights.push_back(light);

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
                                       const std::shared_ptr<Renderer>& renderer,
                                       const uint32_t currentFrame) const
{
  for (auto& light : m_spotLightsToRender)
  {
    const auto spotLight = std::dynamic_pointer_cast<SpotLight>(light);
    if (!spotLight->castsShadows())
    {
      continue;
    }

    const VkExtent2D shadowExtent {
      .width = spotLight->getShadowMapSize(),
      .height = spotLight->getShadowMapSize()
    };

    renderer->beginShadowRendering(0, shadowExtent, commandBuffer, spotLight);

    VkViewport viewport {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(shadowExtent.width),
      .height = static_cast<float>(shadowExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    commandBuffer->setViewport(viewport);

    VkRect2D scissor = {{0, 0}, shadowExtent};
    commandBuffer->setScissor(scissor);

    RenderInfo shadowRenderInfo = {
      .commandBuffer = commandBuffer,
      .currentFrame = currentFrame,
      .viewPosition = spotLight->getPosition(),
      .viewMatrix = spotLight->getLightViewProjectionMatrix(),
      .extent = shadowExtent
    };

    pipelineManager->renderShadowPipeline(commandBuffer, shadowRenderInfo);

    renderer->endShadowRendering(0, commandBuffer);
  }
}

void LightingManager::createUniforms()
{
  m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

  m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(PointLightUniform));

  m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SpotLightUniform));

  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));
}

void LightingManager::createDescriptorSet(VkDescriptorPool descriptorPool)
{
  m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::lightingLayoutBindings);
  m_lightingDescriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_lightMetadataUniform->getDescriptorSet(0, descriptorSet, frame),
      m_pointLightsUniform->getDescriptorSet(1, descriptorSet, frame),
      m_spotLightsUniform->getDescriptorSet(2, descriptorSet, frame),
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame)
    }};

    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    return descriptorWrites;
  });
}

void LightingManager::updateUniforms(const uint32_t currentFrame, const glm::vec3 viewPosition)
{
  const CameraUniform cameraUBO {
    .position = viewPosition
  };
  m_cameraUniform->update(currentFrame, &cameraUBO);

  updatePointLightUniforms(currentFrame);

  updateSpotLightUniforms(currentFrame);
}

void LightingManager::updatePointLightUniforms(const uint32_t currentFrame)
{
  if (m_prevNumPointLights != m_pointLightsToRender.size())
  {
    if (m_pointLightsToRender.empty())
    {
      const LightMetadataUniform lightMetadataUBO {
        .numPointLights = 0,
        .numSpotLights = static_cast<int>(m_spotLightsToRender.size())
      };

      m_lightMetadataUniform->update(currentFrame, &lightMetadataUBO);

      return;
    }

    m_logicalDevice->waitIdle();

    m_pointLightsUniform.reset();

    auto lightsUniformBufferSize = sizeof(PointLightUniform) * m_pointLightsToRender.size();

    m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = static_cast<int>(m_pointLightsToRender.size()),
      .numSpotLights = static_cast<int>(m_spotLightsToRender.size())
    };

    m_lightingDescriptorSet->updateDescriptorSets([this, lightMetadataUBO](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      m_lightMetadataUniform->update(frame, &lightMetadataUBO);

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_pointLightsUniform->getDescriptorSet(1, descriptorSet, frame)
      }};

      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

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
}

void LightingManager::updateSpotLightUniforms(const uint32_t currentFrame)
{
  if (m_prevNumSpotLights != m_spotLightsToRender.size())
  {
    if (m_spotLightsToRender.empty())
    {
      const LightMetadataUniform lightMetadataUBO {
        .numPointLights = static_cast<int>(m_pointLightsToRender.size()),
        .numSpotLights = 0
      };

      m_lightMetadataUniform->update(currentFrame, &lightMetadataUBO);

      return;
    }

    m_logicalDevice->waitIdle();

    m_spotLightsUniform.reset();

    auto lightsUniformBufferSize = sizeof(SpotLightUniform) * m_spotLightsToRender.size();

    m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = static_cast<int>(m_pointLightsToRender.size()),
      .numSpotLights = static_cast<int>(m_spotLightsToRender.size())
    };

    m_lightingDescriptorSet->updateDescriptorSets([this, lightMetadataUBO](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      m_lightMetadataUniform->update(frame, &lightMetadataUBO);

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_spotLightsUniform->getDescriptorSet(2, descriptorSet, frame)
      }};

      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

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
  std::vector<VkDescriptorImageInfo> imageInfos;
  imageInfos.reserve(MAX_SHADOW_MAPS);

  for (auto& light : m_spotLightsToRender)
  {
    const auto spotLight = std::dynamic_pointer_cast<SpotLight>(light);
    if (!spotLight || !spotLight->castsShadows())
    {
      continue;
    }

    imageInfos.push_back({
      m_shadowMapSampler,
      spotLight->getShadowMapView(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
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

  const VkWriteDescriptorSet samplerWrite {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = m_lightingDescriptorSet->getDescriptorSet(currentFrame),
    .dstBinding = 4,
    .dstArrayElement = 0,
    .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = imageInfos.data(),
  };

  m_logicalDevice->updateDescriptorSets(1, &samplerWrite);
}

void LightingManager::createShadowMapSampler()
{
  constexpr VkSamplerCreateInfo samplerInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .compareEnable = VK_TRUE,
    .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    .unnormalizedCoordinates = VK_FALSE
  };

  m_shadowMapSampler = m_logicalDevice->createSampler(samplerInfo);
}

void LightingManager::destroyShadowMapSampler()
{
  m_logicalDevice->destroySampler(m_shadowMapSampler);
}
} // namespace vke