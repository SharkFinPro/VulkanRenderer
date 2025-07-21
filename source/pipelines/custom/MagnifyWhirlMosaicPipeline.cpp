#include "MagnifyWhirlMosaicPipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

constexpr VkDescriptorSetLayoutBinding cameraLayout {
  .binding = 3,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding magnifyWhirlMosaicLayout {
  .binding = 4,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

MagnifyWhirlMosaicPipeline::MagnifyWhirlMosaicPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                       const std::shared_ptr<RenderPass>& renderPass,
                                                       VkDescriptorPool descriptorPool,
                                                       VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice), descriptorPool(descriptorPool),
    objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createGlobalDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

MagnifyWhirlMosaicPipeline::~MagnifyWhirlMosaicPipeline()
{
  m_logicalDevice->destroyDescriptorSetLayout(globalDescriptorSetLayout);
}

void MagnifyWhirlMosaicPipeline::displayGui()
{
  ImGui::Begin("Magnify Whirl Mosaic");

  ImGui::SliderFloat("Lens S Center", &magnifyWhirlMosaicUBO.lensS, 0.0f, 1.0f);
  ImGui::SliderFloat("Lens T Center", &magnifyWhirlMosaicUBO.lensT, 0.0f, 1.0f);
  ImGui::SliderFloat("Lens Radius", &magnifyWhirlMosaicUBO.lensRadius, 0.01f, 0.75f);

  ImGui::Separator();

  ImGui::SliderFloat("Magnification", &magnifyWhirlMosaicUBO.magnification, 0.1f, 7.5f);
  ImGui::SliderFloat("Whirl", &magnifyWhirlMosaicUBO.whirl, -30.0f, 30.0f);
  ImGui::SliderFloat("Mosaic", &magnifyWhirlMosaicUBO.mosaic, 0.001f, 0.1f);

  ImGui::End();
}

void MagnifyWhirlMosaicPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/MagnifyWhirlMosaic.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void MagnifyWhirlMosaicPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
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

void MagnifyWhirlMosaicPipeline::createGlobalDescriptorSetLayout()
{
  constexpr std::array<VkDescriptorSetLayoutBinding, 2> globalBindings {
    cameraLayout,
    magnifyWhirlMosaicLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  globalDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void MagnifyWhirlMosaicPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(m_logicalDevice->getMaxFramesInFlight(), globalDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = m_logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(m_logicalDevice->getMaxFramesInFlight());
  m_logicalDevice->allocateDescriptorSets(allocateInfo, descriptorSets.data());

  for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{{
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      magnifyWhirlMosaicUniform->getDescriptorSet(4, descriptorSets[i], i)
    }};

    m_logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void MagnifyWhirlMosaicPipeline::createUniforms()
{
  cameraUniform = std::make_unique<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  magnifyWhirlMosaicUniform = std::make_unique<UniformBuffer>(m_logicalDevice, sizeof(MagnifyWhirlMosaicUniform));
}

void MagnifyWhirlMosaicPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  magnifyWhirlMosaicUniform->update(renderInfo->currentFrame, &magnifyWhirlMosaicUBO);
}

void MagnifyWhirlMosaicPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &descriptorSets[renderInfo->currentFrame]);
}
