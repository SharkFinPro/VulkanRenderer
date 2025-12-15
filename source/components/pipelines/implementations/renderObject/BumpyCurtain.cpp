#include "BumpyCurtain.h"
#include "../common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../../../renderPass/RenderPass.h"
#include "../../../textures/Texture3D.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include "../../../uniformBuffer/UniformBuffer.h"
#include <imgui.h>

namespace vke {

BumpyCurtain::BumpyCurtain(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           std::shared_ptr<RenderPass> renderPass,
                           const VkCommandPool& commandPool,
                           const VkDescriptorPool descriptorPool,
                           const VkDescriptorSetLayout objectDescriptorSetLayout,
                           const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet),
    m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/Curtain.vert.spv",
      .fragmentShader = "assets/shaders/BumpyCurtain.frag.spv"
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
      m_bumpyCurtainDescriptorSet->getDescriptorSetLayout(),
      m_objectDescriptorSetLayout,
      m_lightingDescriptorSet->getDescriptorSetLayout()
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
}

void BumpyCurtain::displayGui()
{
  ImGui::Begin("Bumpy Curtain");

  ImGui::SliderFloat("Amplitude", &m_curtainUBO.amplitude, 0.001f, 3.0f);
  ImGui::SliderFloat("Period", &m_curtainUBO.period, 0.1f, 10.0f);
  ImGui::SliderFloat("Shininess", &m_curtainUBO.shininess, 1.0f, 100.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &m_noiseOptionsUBO.amplitude, 0.0f, 10.0f);
  ImGui::SliderFloat("Noise Frequency", &m_noiseOptionsUBO.frequency, 0.1f, 10.0f);

  ImGui::End();
}

void BumpyCurtain::createUniforms(const VkCommandPool& commandPool)
{
  m_curtainUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CurtainUniform));

  m_noiseOptionsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(NoiseOptionsUniform));

  m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void BumpyCurtain::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_bumpyCurtainDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::bumpyCurtainLayoutBindings);
  m_bumpyCurtainDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_curtainUniform->getDescriptorSet(4, descriptorSet, frame),
      m_noiseOptionsUniform->getDescriptorSet(6, descriptorSet, frame),
      m_noiseTexture->getDescriptorSet(7, descriptorSet)
    }};

    return descriptorWrites;
  });
}

void BumpyCurtain::updateUniformVariables(const RenderInfo* renderInfo)
{
  m_curtainUniform->update(renderInfo->currentFrame, &m_curtainUBO);

  m_noiseOptionsUniform->update(renderInfo->currentFrame, &m_noiseOptionsUBO);
}

void BumpyCurtain::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_bumpyCurtainDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}

} // namespace vke