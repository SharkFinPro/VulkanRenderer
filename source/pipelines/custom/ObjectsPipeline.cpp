#include "ObjectsPipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include <imgui.h>

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

ObjectsPipeline::ObjectsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
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

ObjectsPipeline::~ObjectsPipeline()
{
  logicalDevice->destroyDescriptorSetLayout(globalDescriptorSetLayout);
}

void ObjectsPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/objects.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ObjectsPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void ObjectsPipeline::defineStates()
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

void ObjectsPipeline::createGlobalDescriptorSetLayout()
{
  constexpr std::array<VkDescriptorSetLayoutBinding, 3> globalBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  globalDescriptorSetLayout = logicalDevice->createDescriptorSetLayout(globalLayoutCreateInfo);
}

void ObjectsPipeline::createDescriptorSets()
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
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i)
    }};

    logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void ObjectsPipeline::createUniforms()
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(CameraUniform));
}

void ObjectsPipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
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

void ObjectsPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);
}

void ObjectsPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                                &descriptorSets[renderInfo->currentFrame]);
}
