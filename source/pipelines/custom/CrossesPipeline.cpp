#include "CrossesPipeline.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include <imgui.h>

CrossesPipeline::CrossesPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkDescriptorPool descriptorPool,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
: GraphicsPipeline(physicalDevice, logicalDevice), descriptorPool(descriptorPool),
  objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createGlobalDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

CrossesPipeline::~CrossesPipeline()
{
  logicalDevice->destroyDescriptorSetLayout(globalDescriptorSetLayout);
}

void CrossesPipeline::displayGui()
{
  ImGui::Begin("Crosses");

  ImGui::SliderInt("Level", &crossesUBO.level, 0, 3);

  ImGui::SliderFloat("Quantize", &crossesUBO.quantize, 2.0f, 50.0f);

  ImGui::SliderFloat("Size", &crossesUBO.size, 0.0001f, 0.1f);

  ImGui::SliderFloat("Shininess", &crossesUBO.shininess, 2.0f, 50.0f);

  ImGui::End();

  ImGui::Begin("Chroma Depth");

  ImGui::Checkbox("Use Chroma Depth", &chromaDepthUBO.use);

  ImGui::SliderFloat("Blue Depth", &chromaDepthUBO.blueDepth, 0.0f, 50.0f);

  ImGui::SliderFloat("Red Depth", &chromaDepthUBO.redDepth, 0.0f, 50.0f);

  ImGui::End();
}

void CrossesPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Crosses.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Crosses.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
  createShader("assets/shaders/Crosses.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void CrossesPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void CrossesPipeline::defineStates()
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

void CrossesPipeline::createGlobalDescriptorSetLayout()
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

  constexpr VkDescriptorSetLayoutBinding crossesLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding chromaDepthLayout {
    .binding = 6,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array globalBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout,
    crossesLayout,
    chromaDepthLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  globalDescriptorSetLayout = logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void CrossesPipeline::createDescriptorSets()
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
    std::array<VkWriteDescriptorSet, 4> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      crossesUniform->getDescriptorSet(4, descriptorSets[i], i),
      chromaDepthUniform->getDescriptorSet(6, descriptorSets[i], i)
    }};

    logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void CrossesPipeline::createUniforms()
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(CameraUniform));

  crossesUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(CrossesUniform));

  chromaDepthUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(ChromaDepthUniform));
}

void CrossesPipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
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

    lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, lightsUniformBufferSize);

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

void CrossesPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  cameraUBO.position = renderInfo->viewPosition;

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);

  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  chromaDepthUniform->update(renderInfo->currentFrame, &chromaDepthUBO);

  crossesUniform->update(renderInfo->currentFrame, &crossesUBO);
}

void CrossesPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  vkCmdBindDescriptorSets(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[renderInfo->currentFrame], 0, nullptr);
}
