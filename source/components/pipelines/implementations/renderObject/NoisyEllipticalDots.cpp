#include "NoisyEllipticalDots.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../descriptorSets/LayoutBindings.h"
#include "../../../renderPass/RenderPass.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include "../../uniformBuffers/UniformBuffer.h"
#include "../../../textures/Texture3D.h"
#include <imgui.h>

namespace vke {

NoisyEllipticalDots::NoisyEllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         std::shared_ptr<RenderPass> renderPass,
                                         const VkCommandPool& commandPool,
                                         const VkDescriptorPool descriptorPool,
                                         const VkDescriptorSetLayout objectDescriptorSetLayout,
                                         const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet)
{
  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/StandardObject.vert.spv",
      .fragmentShader = "assets/shaders/NoisyEllipticalDots.frag.spv"
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
      m_noisyEllipticalDotsDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout,
      m_lightingDescriptorSet->getDescriptorSetLayout()
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
}

void NoisyEllipticalDots::displayGui()
{
  ImGui::Begin("Noisy Elliptical Dots");

  ImGui::SliderFloat("Shininess", &m_ellipticalDotsUBO.shininess, 1.0f, 25.0f);
  ImGui::SliderFloat("S Diameter", &m_ellipticalDotsUBO.sDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("T Diameter", &m_ellipticalDotsUBO.tDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("blendFactor", &m_ellipticalDotsUBO.blendFactor, 0.0f, 1.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &m_noiseOptionsUBO.amplitude, 0.0f, 1.0f);
  ImGui::SliderFloat("Noise Frequency", &m_noiseOptionsUBO.frequency, 0.0f, 10.0f);

  ImGui::End();
}

void NoisyEllipticalDots::createUniforms(const VkCommandPool& commandPool)
{
  m_ellipticalDotsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(EllipticalDotsUniform));

  m_noiseOptionsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(NoiseOptionsUniform));

  m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                               VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void NoisyEllipticalDots::createDescriptorSets(const VkDescriptorPool descriptorPool)
{
  m_noisyEllipticalDotsDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::noisyEllipticalDotsLayoutBindings);
  m_noisyEllipticalDotsDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_ellipticalDotsUniform->getDescriptorSet(4, descriptorSet, frame),
      m_noiseOptionsUniform->getDescriptorSet(6, descriptorSet, frame),
      m_noiseTexture->getDescriptorSet(7, descriptorSet)
    }};

    return descriptorWrites;
  });
}

void NoisyEllipticalDots::updateUniformVariables(const RenderInfo* renderInfo)
{
  m_ellipticalDotsUniform->update(renderInfo->currentFrame, &m_ellipticalDotsUBO);

  m_noiseOptionsUniform->update(renderInfo->currentFrame, &m_noiseOptionsUBO);
}

void NoisyEllipticalDots::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_noisyEllipticalDotsDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}

} // namespace vke