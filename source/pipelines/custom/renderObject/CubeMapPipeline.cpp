#include "CubeMapPipeline.h"
#include "../config/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../../RenderPass.h"
#include "../../../components/textures/TextureCubemap.h"
#include "../../../components/textures/Texture3D.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include "../../../components/UniformBuffer.h"
#include <imgui.h>

CubeMapPipeline::CubeMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkCommandPool& commandPool,
                                 const VkDescriptorPool descriptorPool,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice)
{
  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/StandardObject.vert.spv",
      .fragmentShader = "assets/shaders/CubeMap.frag.spv"
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
      m_cubeMapDescriptorSet->getDescriptorSetLayout(),
      objectDescriptorSetLayout,
    },
    .renderPass = renderPass->getRenderPass()
  };

  createPipeline(graphicsPipelineOptions);
}

void CubeMapPipeline::displayGui()
{
  ImGui::Begin("Cube Map");

  ImGui::SliderFloat("Refract | Reflect -> Blend", &m_cubeMapUBO.mix, 0.0f, 1.0f);
  ImGui::SliderFloat("Index of Refraction", &m_cubeMapUBO.refractionIndex, 0.0f, 5.0f);
  ImGui::SliderFloat("White Mix", &m_cubeMapUBO.whiteMix, 0.0f, 1.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &m_noiseOptionsUBO.amplitude, 0.0f, 5.0f);
  ImGui::SliderFloat("Noise Frequency", &m_noiseOptionsUBO.frequency, 0.0f, 0.5f);

  ImGui::End();
}

void CubeMapPipeline::createUniforms(const VkCommandPool &commandPool)
{
  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  m_cubeMapUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CubeMapUniform));

  m_noiseOptionsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(NoiseOptionsUniform));

  m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT);

  std::array<std::string, 6> paths {
    "assets/cubeMap/nvposx.bmp",
    "assets/cubeMap/nvnegx.bmp",
    "assets/cubeMap/nvposy.bmp",
    "assets/cubeMap/nvnegy.bmp",
    "assets/cubeMap/nvposz.bmp",
    "assets/cubeMap/nvnegz.bmp"
  };
  m_reflectUnit = std::make_shared<TextureCubemap>(m_logicalDevice, commandPool, paths);

  m_refractUnit = std::make_shared<TextureCubemap>(m_logicalDevice, commandPool, paths);
}

void CubeMapPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_cubeMapDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::cubeMapLayoutBindings);
  m_cubeMapDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame),
      m_cubeMapUniform->getDescriptorSet(1, descriptorSet, frame),
      m_noiseOptionsUniform->getDescriptorSet(6, descriptorSet, frame),
      m_noiseTexture->getDescriptorSet(7, descriptorSet),
      m_reflectUnit->getDescriptorSet(4, descriptorSet),
      m_refractUnit->getDescriptorSet(5, descriptorSet)
    }};

    return descriptorWrites;
  });
}

void CubeMapPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  m_cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  m_cubeMapUniform->update(renderInfo->currentFrame, &m_cubeMapUBO);

  m_noiseOptionsUniform->update(renderInfo->currentFrame, &m_noiseOptionsUBO);
}

void CubeMapPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_cubeMapDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
