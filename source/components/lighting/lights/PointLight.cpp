#include "PointLight.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../renderingManager/ImageResource.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vke {

  PointLight::PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
                         const CommonLightData& commonLightData,
                         const vk::CommandPool& commandPool,
                         const vk::DescriptorPool descriptorPool,
                         const vk::DescriptorSetLayout descriptorSetLayout)
    : Light(std::move(logicalDevice), commonLightData)
  {
    PointLight::createShadowMap(commandPool);

    createUniform();

    createDescriptorSet(descriptorPool, descriptorSetLayout);
  }

  LightType PointLight::getLightType() const
  {
    return LightType::pointLight;
  }

  LightUniform PointLight::getUniform() const
  {
    return PointLightUniform {
      .lightViewProjections = getLightViewProjectionMatrices(),
      .position = m_position,
      .color = m_color,
      .ambient = m_ambient,
      .diffuse = m_diffuse,
      .specular = m_specular
    };
  }

  std::array<glm::mat4, 6> PointLight::getLightViewProjectionMatrices() const
  {
    glm::mat4 projection = glm::perspective(
      glm::radians(90.0f),
      1.0f,
      0.1f,
      100.0f
    );

    projection[1][1] *= -1;

    const std::array<glm::mat4, 6> viewMatrices {
      glm::lookAt(m_position, m_position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)),
      glm::lookAt(m_position, m_position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)),

      // Y direction order must be swapped for point light shadow maps
      glm::lookAt(m_position, m_position + glm::vec3(0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)),
      glm::lookAt(m_position, m_position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),

      glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)),
      glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0))
    };

    return {
      projection * viewMatrices[0],
      projection * viewMatrices[1],
      projection * viewMatrices[2],
      projection * viewMatrices[3],
      projection * viewMatrices[4],
      projection * viewMatrices[5],
    };
  }

  void PointLight::updateUniform(const uint32_t currentFrame) const
  {
    const auto matrices = getLightViewProjectionMatrices();
    m_viewProjectionUniform->update(currentFrame, &matrices);
  }

  vk::DescriptorSet PointLight::getDescriptorSet(const uint32_t currentFrame) const
  {
    return m_descriptorSet->getDescriptorSet(currentFrame);
  }

  void PointLight::createShadowMap(const vk::CommandPool commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    ImageResourceConfig imageResourceConfig {
      .imageResourceType = ImageResourceType::Depth,
      .logicalDevice = m_logicalDevice,
      .extent = m_shadowMapExtent,
      .commandPool = commandPool,
      .depthFormat = vk::Format::eD32Sfloat,
      .numSamples = vk::SampleCountFlagBits::e1,
      .isCubeMap = true
    };

    m_shadowMapDepthImageResource = std::make_shared<ImageResource>(imageResourceConfig);
  }

  void PointLight::createUniform()
  {
    m_viewProjectionUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(glm::mat4) * 6);
  }

  void PointLight::createDescriptorSet(vk::DescriptorPool descriptorPool,
                                       vk::DescriptorSetLayout descriptorSetLayout)
  {
    m_descriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, descriptorSetLayout);
    m_descriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<vk::WriteDescriptorSet> descriptorWrites{{
        m_viewProjectionUniform->getDescriptorSet(0, descriptorSet, frame)
      }};

      return descriptorWrites;
    });
  }

} // vke