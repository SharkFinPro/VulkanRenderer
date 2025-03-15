#include "MagnifyWhirlMosaicPipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/RenderObject.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>
#include <stdexcept>

MagnifyWhirlMosaicPipeline::MagnifyWhirlMosaicPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                                       const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                       const std::shared_ptr<RenderPass>& renderPass,
                                                       VkDescriptorPool descriptorPool,
                                                       VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(physicalDevice, logicalDevice), descriptorPool(descriptorPool),
    objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createGlobalDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

MagnifyWhirlMosaicPipeline::~MagnifyWhirlMosaicPipeline()
{
  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), globalDescriptorSetLayout, nullptr);
}

void MagnifyWhirlMosaicPipeline::render(const RenderInfo* renderInfo,
                                        const std::vector<std::shared_ptr<RenderObject>>* objects)
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

  magnifyWhirlMosaicUniform->update(renderInfo->currentFrame, &magnifyWhirlMosaicUBO, sizeof(MagnifyWhirlMosaicUniform));

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
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void MagnifyWhirlMosaicPipeline::createGlobalDescriptorSetLayout()
{
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

  constexpr std::array<VkDescriptorSetLayoutBinding, 2> globalBindings {
    cameraLayout,
    magnifyWhirlMosaicLayout
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

void MagnifyWhirlMosaicPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), globalDescriptorSetLayout);
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
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{{
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      magnifyWhirlMosaicUniform->getDescriptorSet(4, descriptorSets[i], i)
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void MagnifyWhirlMosaicPipeline::createUniforms()
{
  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(CameraUniform));

  magnifyWhirlMosaicUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(MagnifyWhirlMosaicUniform));
}
