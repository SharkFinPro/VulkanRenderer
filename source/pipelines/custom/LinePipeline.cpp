#include "LinePipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include <stdexcept>

LinePipeline::LinePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<RenderPass>& renderPass,
                           VkDescriptorPool descriptorPool)
  : GraphicsPipeline(physicalDevice, logicalDevice), descriptorPool(descriptorPool)
{
  createUniforms();

  createLineDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

LinePipeline::~LinePipeline()
{
  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), lineDescriptorSetLayout, nullptr);
}

void LinePipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Line.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void LinePipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(lineDescriptorSetLayout);
}

void LinePipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateLineList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateLineVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void LinePipeline::createLineDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 1> lineBindings {
    transformLayout
  };

  const VkDescriptorSetLayoutCreateInfo lineLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(lineBindings.size()),
    .pBindings = lineBindings.data()
  };

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &lineLayoutCreateInfo, nullptr, &lineDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create line descriptor set layout!");
  }
}

void LinePipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), lineDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(logicalDevice->getMaxFramesInFlight());
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::array<VkWriteDescriptorSet, 1> descriptorWrites{{
      transformUniform->getDescriptorSet(0, descriptorSets[i], i),
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void LinePipeline::createUniforms()
{
  transformUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(TransformUniform));
}

void LinePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const TransformUniform transformUBO {
    // .model = createModelMatrix(),
    .model = glm::mat4(1.0f),
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->projectionMatrix
  };

  transformUniform->update(renderInfo->currentFrame, &transformUBO);
}

void LinePipeline::bindDescriptorSet(const RenderInfo *renderInfo)
{
  vkCmdBindDescriptorSets(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[renderInfo->currentFrame], 0, nullptr);
}
