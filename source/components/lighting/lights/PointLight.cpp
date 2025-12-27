#include "PointLight.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../renderingManager/ImageResource.h"
#include "../../renderingManager/RenderTarget.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vke {

  PointLight::PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
                         const glm::vec3& position,
                         const glm::vec3& color,
                         const float ambient,
                         const float diffuse,
                         const float specular,
                         const VkCommandPool& commandPool,
                         VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout,
                         const std::shared_ptr<Renderer>& renderer)
    : Light(std::move(logicalDevice), position, color, ambient, diffuse, specular)
  {
    PointLight::createShadowMap(commandPool);

    registerShadowMapRenderTarget(renderer);

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

  void PointLight::bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     const VkPipelineLayout& pipelineLayout,
                                     const uint32_t currentFrame) const
  {
    commandBuffer->bindDescriptorSets(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      1,
      1,
      &m_descriptorSet->getDescriptorSet(currentFrame)
    );
  }

  void PointLight::createShadowMap(const VkCommandPool& commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = {
        .width = m_shadowMapSize,
        .height = m_shadowMapSize
      },
      .commandPool = commandPool,
      .depthFormat = VK_FORMAT_D32_SFLOAT,
      .numSamples = VK_SAMPLE_COUNT_1_BIT,
      .isCubeMap = true
    };

    m_shadowMapRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig);
  }

  void PointLight::createUniform()
  {
    m_viewProjectionUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(glm::mat4) * 6);
  }

  void PointLight::createDescriptorSet(VkDescriptorPool descriptorPool,
                                       VkDescriptorSetLayout descriptorSetLayout)
  {
    m_descriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, descriptorSetLayout);
    m_descriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_viewProjectionUniform->getDescriptorSet(0, descriptorSet, frame)
      }};

      return descriptorWrites;
    });
  }

} // vke