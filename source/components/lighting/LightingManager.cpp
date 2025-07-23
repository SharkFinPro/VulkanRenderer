#include "LightingManager.h"
#include "Light.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../pipelines/custom/config/Uniforms.h"
#include "../../pipelines/custom/descriptorSets/DescriptorSet.h"
#include "../../pipelines/custom/descriptorSets/LayoutBindings.h"
#include "../../objects/UniformBuffer.h"

LightingManager::LightingManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDescriptorPool descriptorPool)
  : m_logicalDevice(logicalDevice)
{
  createUniforms();

  createDescriptorSet(descriptorPool);
}

std::shared_ptr<Light> LightingManager::createLight(glm::vec3 position,
                                                    glm::vec3 color,
                                                    float ambient,
                                                    float diffuse,
                                                    float specular)
{
  auto light = std::make_shared<Light>(position, color, ambient, diffuse, specular);

  lights.push_back(light);

  return light;
}

void LightingManager::renderLight(const std::shared_ptr<Light>& light)
{
  if (light->isSpotLight())
  {
    spotLightsToRender.push_back(light);
  }
  else
  {
    pointLightsToRender.push_back(light);
  }
}

std::shared_ptr<DescriptorSet> LightingManager::getLightingDescriptorSet() const
{
  return m_lightingDescriptorSet;
}

void LightingManager::clearLightsToRender()
{
  pointLightsToRender.clear();
  spotLightsToRender.clear();
}

void LightingManager::update(const uint32_t currentFrame, const glm::vec3 viewPosition)
{
  updateUniforms(currentFrame, viewPosition);
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
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame)
    }};

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
  if (m_prevNumPointLights != pointLightsToRender.size())
  {
    m_logicalDevice->waitIdle();

    m_pointLightsUniform.reset();

    auto lightsUniformBufferSize = sizeof(PointLightUniform) * pointLightsToRender.size();

    m_pointLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = static_cast<int>(pointLightsToRender.size()),
      .numSpotLights = static_cast<int>(spotLightsToRender.size())
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

    m_prevNumPointLights = static_cast<int>(pointLightsToRender.size());
  }

  if (pointLightsToRender.empty())
  {
    return;
  }

  std::vector<PointLightUniform> lightUniforms;
  lightUniforms.resize(pointLightsToRender.size());
  for (int i = 0; i < pointLightsToRender.size(); i++)
  {
    lightUniforms[i] = pointLightsToRender[i]->getPointLightUniform();
  }

  m_pointLightsUniform->update(currentFrame, lightUniforms.data());
}

void LightingManager::updateSpotLightUniforms(const uint32_t currentFrame)
{
  if (m_prevNumSpotLights != spotLightsToRender.size())
  {
    m_logicalDevice->waitIdle();

    m_spotLightsUniform.reset();

    auto lightsUniformBufferSize = sizeof(SpotLightUniform) * spotLightsToRender.size();

    m_spotLightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    const LightMetadataUniform lightMetadataUBO {
      .numPointLights = static_cast<int>(spotLightsToRender.size()),
      .numSpotLights = static_cast<int>(spotLightsToRender.size())
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

    m_prevNumSpotLights = static_cast<int>(spotLightsToRender.size());
  }

  if (spotLightsToRender.empty())
  {
    return;
  }

  std::vector<SpotLightUniform> lightUniforms;
  lightUniforms.resize(spotLightsToRender.size());
  for (int i = 0; i < spotLightsToRender.size(); i++)
  {
    lightUniforms[i] = spotLightsToRender[i]->getSpotLightUniform();
  }

  m_spotLightsUniform->update(currentFrame, lightUniforms.data());
}
