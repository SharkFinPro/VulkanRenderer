#include "TexturedPlane.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/RenderObject.h"
#include "../../objects/UniformBuffer.h"
#include <stdexcept>

TexturedPlane::TexturedPlane(const std::shared_ptr<PhysicalDevice>& physicalDevice,
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

TexturedPlane::~TexturedPlane()
{
  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), globalDescriptorSetLayout, nullptr);
}

void TexturedPlane::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  vkCmdBindPipeline(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const VkViewport viewport {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(renderInfo->extent.width),
    .height = static_cast<float>(renderInfo->extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(renderInfo->commandBuffer, 0, 1, &viewport);

  const VkRect2D scissor {
    .offset = {0, 0},
    .extent = renderInfo->extent
  };
  vkCmdSetScissor(renderInfo->commandBuffer, 0, 1, &scissor);

  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO, sizeof(CameraUniform));

  vkCmdBindDescriptorSets(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[renderInfo->currentFrame], 0, nullptr);

  glm::mat4 projectionMatrix = glm::perspective(
    glm::radians(45.0f),
    static_cast<float>(renderInfo->extent.width) / static_cast<float>(renderInfo->extent.height),
    0.1f,
    1000.0f
  );

  projectionMatrix[1][1] *= -1;

  for (const auto& object : *objects)
  {
    object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, projectionMatrix);

    object->draw(renderInfo->commandBuffer, pipelineLayout, renderInfo->currentFrame);
  }
}

void TexturedPlane::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/TexturedPlane.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void TexturedPlane::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void TexturedPlane::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void TexturedPlane::createGlobalDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding cameraLayout {
    .binding = 3,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 1> globalBindings {
    cameraLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &globalLayoutCreateInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create global descriptor set layout!");
  }
}

void TexturedPlane::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), globalDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(logicalDevice->getMaxFramesInFlight()),
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
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i)
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }

}

void TexturedPlane::createUniforms()
{
  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(CameraUniform));
}
