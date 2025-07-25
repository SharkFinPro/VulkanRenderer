#include "MagnifyWhirlMosaicPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "config/Uniforms.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../core/commandBuffer/CommandBuffer.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

MagnifyWhirlMosaicPipeline::MagnifyWhirlMosaicPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                       const std::shared_ptr<RenderPass>& renderPass,
                                                       VkDescriptorPool descriptorPool,
                                                       VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice),
    m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void MagnifyWhirlMosaicPipeline::displayGui()
{
  ImGui::Begin("Magnify Whirl Mosaic");

  ImGui::SliderFloat("Lens S Center", &m_magnifyWhirlMosaicUBO.lensS, 0.0f, 1.0f);
  ImGui::SliderFloat("Lens T Center", &m_magnifyWhirlMosaicUBO.lensT, 0.0f, 1.0f);
  ImGui::SliderFloat("Lens Radius", &m_magnifyWhirlMosaicUBO.lensRadius, 0.01f, 0.75f);

  ImGui::Separator();

  ImGui::SliderFloat("Magnification", &m_magnifyWhirlMosaicUBO.magnification, 0.1f, 7.5f);
  ImGui::SliderFloat("Whirl", &m_magnifyWhirlMosaicUBO.whirl, -30.0f, 30.0f);
  ImGui::SliderFloat("Mosaic", &m_magnifyWhirlMosaicUBO.mosaic, 0.001f, 0.1f);

  ImGui::End();
}

void MagnifyWhirlMosaicPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/MagnifyWhirlMosaic.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void MagnifyWhirlMosaicPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_magnifyWhirlMosaicDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
}

void MagnifyWhirlMosaicPipeline::defineStates()
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

void MagnifyWhirlMosaicPipeline::createUniforms()
{
  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  m_magnifyWhirlMosaicUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(MagnifyWhirlMosaicUniform));
}

void MagnifyWhirlMosaicPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_magnifyWhirlMosaicDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::magnifyWhirlMosaicLayoutBindings);
  m_magnifyWhirlMosaicDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame),
      m_magnifyWhirlMosaicUniform->getDescriptorSet(4, descriptorSet, frame)
    }};

    return descriptorWrites;
  });
}

void MagnifyWhirlMosaicPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  m_cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  m_magnifyWhirlMosaicUniform->update(renderInfo->currentFrame, &m_magnifyWhirlMosaicUBO);
}

void MagnifyWhirlMosaicPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_magnifyWhirlMosaicDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
