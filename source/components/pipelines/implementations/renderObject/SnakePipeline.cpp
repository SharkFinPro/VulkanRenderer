#include "SnakePipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../../renderPass/RenderPass.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include <imgui.h>

namespace vke {

SnakePipeline::SnakePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass,
                             const VkDescriptorSetLayout objectDescriptorSetLayout,
                             const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet)
{
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
    .pushConstantRanges {
      {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(SnakeUniform)
      }
    },
    .descriptorSetLayouts {
      m_lightingDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout
    },
    .renderPass = renderPass
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

void SnakePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}

void SnakePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                           0, sizeof(SnakeUniform), &m_snakeUBO);
}

} // namespace vke