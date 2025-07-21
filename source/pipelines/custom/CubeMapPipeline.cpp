#include "CubeMapPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../components/textures/TextureCubemap.h"
#include "../../components/textures/Texture3D.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

constexpr VkDescriptorSetLayoutBinding cameraLayout {
  .binding = 0,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding cubeMapLayout {
  .binding = 1,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding noiseOptionsLayout {
  .binding = 2,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding noiseSamplerLayout {
  .binding = 3,
  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding reflectUnitLayout {
  .binding = 4,
  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

constexpr VkDescriptorSetLayoutBinding refractUnitLayout {
  .binding = 5,
  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  .descriptorCount = 1,
  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
};

CubeMapPipeline::CubeMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkCommandPool& commandPool,
                                 const VkDescriptorPool descriptorPool,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice), descriptorPool(descriptorPool),
    objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms(commandPool);

  createGlobalDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

CubeMapPipeline::~CubeMapPipeline()
{
  m_logicalDevice->destroyDescriptorSetLayout(globalDescriptorSetLayout);
}

void CubeMapPipeline::displayGui()
{
  ImGui::Begin("Cube Map");

  ImGui::SliderFloat("Refract | Reflect -> Blend", &cubeMapUBO.mix, 0.0f, 1.0f);
  ImGui::SliderFloat("Index of Refraction", &cubeMapUBO.refractionIndex, 0.0f, 5.0f);
  ImGui::SliderFloat("White Mix", &cubeMapUBO.whiteMix, 0.0f, 1.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &noiseOptionsUBO.amplitude, 0.0f, 5.0f);
  ImGui::SliderFloat("Noise Frequency", &noiseOptionsUBO.frequency, 0.0f, 0.5f);

  ImGui::End();
}

void CubeMapPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/CubeMap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void CubeMapPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void CubeMapPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void CubeMapPipeline::createGlobalDescriptorSetLayout()
{
  constexpr std::array globalBindings {
    cameraLayout,
    cubeMapLayout,
    noiseOptionsLayout,
    noiseSamplerLayout,
    reflectUnitLayout,
    refractUnitLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  globalDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void CubeMapPipeline::createDescriptorSets()
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
    std::array<VkWriteDescriptorSet, 6> descriptorWrites{{
      cameraUniform->getDescriptorSet(0, descriptorSets[i], i),
      cubeMapUniform->getDescriptorSet(1, descriptorSets[i], i),
      noiseOptionsUniform->getDescriptorSet(2, descriptorSets[i], i),
      noiseTexture->getDescriptorSet(3, descriptorSets[i]),
      reflectUnit->getDescriptorSet(4, descriptorSets[i]),
      refractUnit->getDescriptorSet(5, descriptorSets[i])
    }};

    m_logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void CubeMapPipeline::createUniforms(const VkCommandPool &commandPool)
{
  cameraUniform = std::make_unique<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  cubeMapUniform = std::make_unique<UniformBuffer>(m_logicalDevice, sizeof(CubeMapUniform));

  noiseOptionsUniform = std::make_unique<UniformBuffer>(m_logicalDevice, sizeof(NoiseOptionsUniform));

  noiseTexture = std::make_unique<Texture3D>(m_logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT);

  std::array<std::string, 6> paths {
    "assets/cubeMap/nvposx.bmp",
    "assets/cubeMap/nvnegx.bmp",
    "assets/cubeMap/nvposy.bmp",
    "assets/cubeMap/nvnegy.bmp",
    "assets/cubeMap/nvposz.bmp",
    "assets/cubeMap/nvnegz.bmp"
  };
  reflectUnit = std::make_unique<TextureCubemap>(m_logicalDevice, commandPool, paths);

  refractUnit = std::make_unique<TextureCubemap>(m_logicalDevice, commandPool, paths);
}

void CubeMapPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  cubeMapUniform->update(renderInfo->currentFrame, &cubeMapUBO);

  noiseOptionsUniform->update(renderInfo->currentFrame, &noiseOptionsUBO);
}

void CubeMapPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &descriptorSets[renderInfo->currentFrame]);
}
