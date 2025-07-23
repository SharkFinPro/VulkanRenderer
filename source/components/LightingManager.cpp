#include "LightingManager.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../pipelines/custom/config/Uniforms.h"
#include "../pipelines/custom/descriptorSets/DescriptorSet.h"
#include "../pipelines/custom/descriptorSets/LayoutBindings.h"
#include "../objects/Light.h"
#include "../objects/UniformBuffer.h"

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
  lightsToRender.push_back(light);
}

std::shared_ptr<DescriptorSet> LightingManager::getLightingDescriptorSet() const
{
  return m_lightingDescriptorSet;
}

void LightingManager::clearLightsToRender()
{
  lightsToRender.clear();
}

void LightingManager::update(const uint32_t currentFrame, const glm::vec3 viewPosition)
{
  updateUniforms(currentFrame, viewPosition);
}

void LightingManager::createUniforms()
{
  m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

  m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightUniform));

  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));
}

void LightingManager::createDescriptorSet(VkDescriptorPool descriptorPool)
{
  m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::lightingLayoutBindings);
  m_lightingDescriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_lightMetadataUniform->getDescriptorSet(2, descriptorSet, frame),
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

  if (lights.empty())
  {
    return;
  }

  if (m_prevNumLights != lights.size())
  {
    m_logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    m_lightsUniform.reset();

    auto lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    m_lightingDescriptorSet->updateDescriptorSets([this, lightMetadataUBO](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      m_lightMetadataUniform->update(frame, &lightMetadataUBO);

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_lightsUniform->getDescriptorSet(5, descriptorSet, frame)
      }};

      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      return descriptorWrites;
    });

    m_prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  m_lightsUniform->update(currentFrame, lightUniforms.data());
}
