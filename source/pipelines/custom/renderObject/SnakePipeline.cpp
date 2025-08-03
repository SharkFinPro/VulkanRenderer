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
    m_lightingDescriptorSet(lightingDescriptorSet)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/Snake.vert.spv",
      .geometryShader = "assets/shaders/Snake.geom.spv",
      .fragmentShader = "assets/shaders/Snake.frag.spv"
    },
    .states {
      .colorBlendState = GraphicsPipelineStates::colorBlendState,
      .depthStencilState = GraphicsPipelineStates::depthStencilState,
      .dynamicState = GraphicsPipelineStates::dynamicState,
      .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
      .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
      .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
      .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
      .viewportState = GraphicsPipelineStates::viewportState
    },
    .descriptorSetLayouts {
      m_snakeDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout,
      m_lightingDescriptorSet->getDescriptorSetLayout()
    },
    .renderPass = renderPass->getRenderPass()
  };

  createPipeline(graphicsPipelineOptions);
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
