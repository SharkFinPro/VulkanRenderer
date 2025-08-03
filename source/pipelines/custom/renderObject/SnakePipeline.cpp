#include "SnakePipeline.h"
#include "../config/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../../RenderPass.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include "../../../components/UniformBuffer.h"
#include <imgui.h>

SnakePipeline::SnakePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const VkDescriptorPool descriptorPool,
                             const VkDescriptorSetLayout objectDescriptorSetLayout,
                             const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet),
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
  m_snakeUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SnakeUniform));
}

void SnakePipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_snakeDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::snakeLayoutBindings);
  m_snakeDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_snakeUniform->getDescriptorSet(4, descriptorSet, frame)
    }};

    return descriptorWrites;
  });
}

void SnakePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  m_snakeUniform->update(renderInfo->currentFrame, &m_snakeUBO);
}

void SnakePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_snakeDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
