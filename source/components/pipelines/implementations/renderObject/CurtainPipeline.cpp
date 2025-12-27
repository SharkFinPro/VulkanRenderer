#include "CurtainPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../descriptorSets/LayoutBindings.h"
#include "../../uniformBuffers/UniformBuffer.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include <imgui.h>

namespace vke {

  CurtainPipeline::CurtainPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                   std::shared_ptr<RenderPass> renderPass,
                                   const VkDescriptorPool descriptorPool,
                                   const VkDescriptorSetLayout objectDescriptorSetLayout,
                                   const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
    : GraphicsPipeline(std::move(logicalDevice)), m_lightingDescriptorSet(lightingDescriptorSet)
  {
    createUniforms();

    createDescriptorSets(descriptorPool);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Curtain.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/Curtain.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        m_curtainDescriptorSet->getDescriptorSetLayout(),
        objectDescriptorSetLayout,
        m_lightingDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void CurtainPipeline::displayGui()
  {
    ImGui::Begin("Curtain");

    ImGui::SliderFloat("Amplitude", &m_curtainUBO.amplitude, 0.001f, 3.0f);
    ImGui::SliderFloat("Period", &m_curtainUBO.period, 0.1f, 10.0f);
    ImGui::SliderFloat("Shininess", &m_curtainUBO.shininess, 1.0f, 100.0f);

    ImGui::End();
  }

  void CurtainPipeline::createUniforms()
  {
    m_curtainUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CurtainUniform));
  }

  void CurtainPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
  {
    m_curtainDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::curtainLayoutBindings);
    m_curtainDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_curtainUniform->getDescriptorSet(4, descriptorSet, frame),
      }};

      return descriptorWrites;
    });
  }

  void CurtainPipeline::updateUniformVariables(const RenderInfo* renderInfo)
  {
    m_curtainUniform->update(renderInfo->currentFrame, &m_curtainUBO);
  }

  void CurtainPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                  &m_curtainDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                  &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }

} // namespace vke