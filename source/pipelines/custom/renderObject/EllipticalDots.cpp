#include "EllipticalDots.h"
#include "../config/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../RenderPass.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include <imgui.h>

EllipticalDots::EllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const std::shared_ptr<RenderPass>& renderPass,
                               const VkDescriptorSetLayout objectDescriptorSetLayout,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet)
{
  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/StandardObject.vert.spv",
      .fragmentShader = "assets/shaders/EllipticalDots.frag.spv"
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
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(EllipticalDotsUniform)
      }
    },
    .descriptorSetLayouts {
      m_lightingDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout
    },
    .renderPass = renderPass->getRenderPass()
  };

  createPipeline(graphicsPipelineOptions);
}

void EllipticalDots::displayGui()
{
  ImGui::Begin("Elliptical Dots");

  ImGui::SliderFloat("Shininess", &m_ellipticalDotsUBO.shininess, 1.0f, 25.0f);
  ImGui::SliderFloat("S Diameter", &m_ellipticalDotsUBO.sDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("T Diameter", &m_ellipticalDotsUBO.tDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("blendFactor", &m_ellipticalDotsUBO.blendFactor, 0.0f, 1.0f);

  ImGui::End();
}

void EllipticalDots::updateUniformVariables(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                           sizeof(EllipticalDotsUniform), &m_ellipticalDotsUBO);
}

void EllipticalDots::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
