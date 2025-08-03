#include "EllipticalDots.h"
#include "../config/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../../RenderPass.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include "../../../components/UniformBuffer.h"
#include <imgui.h>

EllipticalDots::EllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
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

void EllipticalDots::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_ellipticalDotsDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
}

void EllipticalDots::createUniforms()
{
  m_ellipticalDotsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(EllipticalDotsUniform));
}

void EllipticalDots::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_ellipticalDotsDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::ellipticalDotsLayoutBindings);
  m_ellipticalDotsDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_ellipticalDotsUniform->getDescriptorSet(4, descriptorSet, frame)
    }};

    return descriptorWrites;
  });
}

void EllipticalDots::updateUniformVariables(const RenderInfo* renderInfo)
{
  m_ellipticalDotsUniform->update(renderInfo->currentFrame, &m_ellipticalDotsUBO);
}

void EllipticalDots::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_ellipticalDotsDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
