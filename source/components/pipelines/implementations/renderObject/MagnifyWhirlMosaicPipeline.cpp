#include "MagnifyWhirlMosaicPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../common/Uniforms.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../descriptorSets/LayoutBindings.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include "../../../pipelines/uniformBuffers/UniformBuffer.h"
#include <imgui.h>

namespace vke {

MagnifyWhirlMosaicPipeline::MagnifyWhirlMosaicPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                       std::shared_ptr<RenderPass> renderPass,
                                                       VkDescriptorPool descriptorPool,
                                                       VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
      .fragmentShader = "assets/shaders/renderObject/MagnifyWhirlMosaic.frag.spv"
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
      m_magnifyWhirlMosaicDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
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

} // namespace vke
