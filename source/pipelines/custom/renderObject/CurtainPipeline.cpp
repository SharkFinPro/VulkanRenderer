#include "CurtainPipeline.h"
#include "../config/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../../RenderPass.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include "../../../components/UniformBuffer.h"
#include <imgui.h>

CurtainPipeline::CurtainPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
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

void CurtainPipeline::displayGui()
{
  ImGui::Begin("Curtain");

  ImGui::SliderFloat("Amplitude", &m_curtainUBO.amplitude, 0.001f, 3.0f);
  ImGui::SliderFloat("Period", &m_curtainUBO.period, 0.1f, 10.0f);
  ImGui::SliderFloat("Shininess", &m_curtainUBO.shininess, 1.0f, 100.0f);

  ImGui::End();
}

void CurtainPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Curtain.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Curtain.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void CurtainPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_curtainDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
}

void CurtainPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
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
