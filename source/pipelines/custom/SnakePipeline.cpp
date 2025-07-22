#include "SnakePipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include <imgui.h>

SnakePipeline::SnakePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const VkDescriptorPool descriptorPool,
                             const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice),
    m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void SnakePipeline::displayGui()
{
  ImGui::Begin("Snake");

  ImGui::SliderFloat("Wiggle", &m_snakeUBO.wiggle, -1.0f, 1.0f);

  ImGui::End();

  static float w = 0.0f;
  w += 0.025f;

  m_snakeUBO.wiggle = sin(w);
}

void SnakePipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Snake.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Snake.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
  createShader("assets/shaders/Snake.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void SnakePipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_snakeDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
}

void SnakePipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void SnakePipeline::createUniforms()
{
  m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

  m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightUniform));

  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  m_snakeUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SnakeUniform));
}

void SnakePipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::lightingLayoutBindings);
  m_lightingDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_lightMetadataUniform->getDescriptorSet(2, descriptorSet, frame),
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame)
    }};

    return descriptorWrites;
  });

  m_snakeDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::snakeLayoutBindings);
  m_snakeDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_snakeUniform->getDescriptorSet(4, descriptorSet, frame)
    }};

    return descriptorWrites;
  });
}

void SnakePipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
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

    m_lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, m_lightsUniformBufferSize);

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

void SnakePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  m_cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);

  m_snakeUniform->update(renderInfo->currentFrame, &m_snakeUBO);
}

void SnakePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_snakeDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
