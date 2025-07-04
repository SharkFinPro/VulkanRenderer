#include "NoisyEllipticalDots.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../components/Camera.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include "../../components/textures/Texture3D.h"
#include <imgui.h>

NoisyEllipticalDots::NoisyEllipticalDots(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                         const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         const std::shared_ptr<RenderPass>& renderPass,
                                         const VkCommandPool& commandPool,
                                         const VkDescriptorPool descriptorPool,
                                         const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(physicalDevice, logicalDevice), descriptorPool(descriptorPool),
    objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms(commandPool);

  createGlobalDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

NoisyEllipticalDots::~NoisyEllipticalDots()
{
  logicalDevice->destroyDescriptorSetLayout(globalDescriptorSetLayout);
}

void NoisyEllipticalDots::displayGui()
{
  ImGui::Begin("Noisy Elliptical Dots");

  ImGui::SliderFloat("Shininess", &ellipticalDotsUBO.shininess, 1.0f, 25.0f);
  ImGui::SliderFloat("S Diameter", &ellipticalDotsUBO.sDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("T Diameter", &ellipticalDotsUBO.tDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("blendFactor", &ellipticalDotsUBO.blendFactor, 0.0f, 1.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &noiseOptionsUBO.amplitude, 0.0f, 1.0f);
  ImGui::SliderFloat("Noise Frequency", &noiseOptionsUBO.frequency, 0.0f, 10.0f);

  ImGui::End();
}

void NoisyEllipticalDots::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/NoisyEllipticalDots.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void NoisyEllipticalDots::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void NoisyEllipticalDots::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void NoisyEllipticalDots::createGlobalDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding lightMetadataLayout {
    .binding = 2,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding lightsLayout {
    .binding = 5,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding cameraLayout {
    .binding = 3,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding ellipticalDotsLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding noiseOptionsLayout {
    .binding = 6,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding noiseSamplerLayout {
    .binding = 7,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 6> globalBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout,
    ellipticalDotsLayout,
    noiseOptionsLayout,
    noiseSamplerLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  globalDescriptorSetLayout = logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void NoisyEllipticalDots::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), globalDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(logicalDevice->getMaxFramesInFlight());
  logicalDevice->allocateDescriptorSets(allocateInfo, descriptorSets.data());

  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::array<VkWriteDescriptorSet, 5> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      ellipticalDotsUniform->getDescriptorSet(4, descriptorSets[i], i),
      noiseOptionsUniform->getDescriptorSet(6, descriptorSets[i], i),
      noiseTexture->getDescriptorSet(7, descriptorSets[i])
    }};

    logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void NoisyEllipticalDots::createUniforms(const VkCommandPool& commandPool)
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(CameraUniform));

  ellipticalDotsUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(EllipticalDotsUniform));

  noiseOptionsUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(NoiseOptionsUniform));

  noiseTexture = std::make_unique<Texture3D>(logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void NoisyEllipticalDots::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (prevNumLights != lights.size())
  {
    logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    lightsUniform.reset();

    lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, lightsUniformBufferSize);

    for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
    {
      lightMetadataUniform->update(i, &lightMetadataUBO);

      auto descriptorSet = lightsUniform->getDescriptorSet(5, descriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      logicalDevice->updateDescriptorSets(1, &descriptorSet);
    }

    prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  lightsUniform->update(currentFrame, lightUniforms.data());
}

void NoisyEllipticalDots::updateUniformVariables(const RenderInfo *renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);

  ellipticalDotsUniform->update(renderInfo->currentFrame, &ellipticalDotsUBO);

  noiseOptionsUniform->update(renderInfo->currentFrame, &noiseOptionsUBO);
}

void NoisyEllipticalDots::bindDescriptorSet(const RenderInfo *renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                                &descriptorSets[renderInfo->currentFrame]);
}
