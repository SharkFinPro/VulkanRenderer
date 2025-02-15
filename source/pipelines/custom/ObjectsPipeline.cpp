#include "ObjectsPipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/RenderObject.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include <imgui.h>
#include <stdexcept>

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

ObjectsPipeline::ObjectsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(physicalDevice, logicalDevice), objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  createGlobalDescriptorSetLayout();

  createDescriptorPool();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

ObjectsPipeline::~ObjectsPipeline()
{
  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), globalDescriptorSetLayout, nullptr);
}

void ObjectsPipeline::render(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame,
                             const glm::vec3 viewPosition, const glm::mat4& viewMatrix,
                             const VkExtent2D swapChainExtent, const std::vector<std::shared_ptr<Light>>& lights,
                             const std::vector<std::shared_ptr<RenderObject>>& objects)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const VkViewport viewport {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(swapChainExtent.width),
    .height = static_cast<float>(swapChainExtent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  const VkRect2D scissor {
    .offset = {0, 0},
    .extent = swapChainExtent
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  const CameraUniform cameraUBO {
    .position = viewPosition
  };
  cameraUniform->update(currentFrame, &cameraUBO, sizeof(CameraUniform));

  updateLightUniforms(lights, currentFrame);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, nullptr);

  glm::mat4 projectionMatrix = glm::perspective(
    glm::radians(45.0f),
    static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
    0.1f,
    1000.0f
  );

  projectionMatrix[1][1] *= -1;

  for (const auto& object : objects)
  {
    object->updateUniformBuffer(currentFrame, viewMatrix, projectionMatrix);

    object->draw(commandBuffer, pipelineLayout, currentFrame);
  }
}

void ObjectsPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/objects.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
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

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &globalLayoutCreateInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create global descriptor set layout!");
  }
}

void ObjectsPipeline::createDescriptorPool()
{
  constexpr std::array<VkDescriptorPoolSize, 2> poolSizes {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT * 2},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT * 1}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = MAX_FRAMES_IN_FLIGHT,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void ObjectsPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i)
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void ObjectsPipeline::createUniforms()
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                         sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(CameraUniform));
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

    lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                    lightsUniformBufferSize);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      lightMetadataUniform->update(i, &lightMetadataUBO, sizeof(lightMetadataUBO));

      auto descriptorSet = lightsUniform->getDescriptorSet(5, descriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      vkUpdateDescriptorSets(this->logicalDevice->getDevice(), 1, &descriptorSet, 0, nullptr);
    }

    prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  lightsUniform->update(currentFrame, lightUniforms.data(), lightsUniformBufferSize);
}
