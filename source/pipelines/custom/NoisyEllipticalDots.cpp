#include "NoisyEllipticalDots.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../objects/RenderObject.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include "../../objects/Noise3DTexture.h"
#include <imgui.h>
#include <stdexcept>

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

NoisyEllipticalDots::NoisyEllipticalDots(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                         const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         const std::shared_ptr<RenderPass>& renderPass, const VkCommandPool& commandPool)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
  createUniforms(commandPool);

  createDescriptorSetLayouts();

  createDescriptorPool();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

NoisyEllipticalDots::~NoisyEllipticalDots()
{
  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), objectDescriptorSetLayout, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), globalDescriptorSetLayout, nullptr);
}

VkDescriptorSetLayout& NoisyEllipticalDots::getLayout()
{
  return objectDescriptorSetLayout;
}

void NoisyEllipticalDots::render(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame,
                                 const glm::vec3 viewPosition, const glm::mat4& viewMatrix, const VkExtent2D swapChainExtent,
                                 const std::vector<std::shared_ptr<Light>>& lights,
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

  ImGui::Begin("Elliptical Dots");

  ImGui::SliderFloat("Shininess", &ellipticalDotsUBO.shininess, 1.0f, 25.0f);
  ImGui::SliderFloat("S Diameter", &ellipticalDotsUBO.sDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("T Diameter", &ellipticalDotsUBO.tDiameter, 0.001f, 0.5f);
  ImGui::SliderFloat("blendFactor", &ellipticalDotsUBO.blendFactor, 0.0f, 1.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &noiseOptionsUBO.amplitude, 0.0f, 1.0f);
  ImGui::SliderFloat("Noise Frequency", &noiseOptionsUBO.frequency, 0.0f, 10.0f);

  ImGui::End();
  ellipticalDotsUniform->update(currentFrame, &ellipticalDotsUBO, sizeof(EllipticalDotsUniform));
  noiseOptionsUniform->update(currentFrame, &noiseOptionsUBO, sizeof(NoiseOptionsUniform));

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

void NoisyEllipticalDots::loadGraphicsShaders()
{
  createShader("assets/shaders/EllipticalDots.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
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

void NoisyEllipticalDots::createDescriptorSetLayouts()
{
  createGlobalDescriptorSetLayout();
  createObjectDescriptorSetLayout();
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

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &globalLayoutCreateInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create global descriptor set layout!");
  }
}

void NoisyEllipticalDots::createObjectDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding textureLayout {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding specularLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 3> objectBindings {
    transformLayout,
    textureLayout,
    specularLayout
  };

  const VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(objectBindings.size()),
    .pBindings = objectBindings.data()
  };

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &objectLayoutCreateInfo, nullptr,
                                  &objectDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create object descriptor set layout!");
  }
}

void NoisyEllipticalDots::createDescriptorPool()
{
  constexpr std::array<VkDescriptorPoolSize, 3> poolSizes {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT * 4},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT * 1},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT}
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

void NoisyEllipticalDots::createDescriptorSets()
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
    std::array<VkWriteDescriptorSet, 5> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      ellipticalDotsUniform->getDescriptorSet(4, descriptorSets[i], i),
      noiseOptionsUniform->getDescriptorSet(6, descriptorSets[i], i),
      noiseTexture->getDescriptorSet(7, descriptorSets[i])
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void NoisyEllipticalDots::createUniforms(const VkCommandPool& commandPool)
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                         sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(CameraUniform));

  ellipticalDotsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                          sizeof(EllipticalDotsUniform));

  noiseOptionsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                        sizeof(NoiseOptionsUniform));

  noiseTexture = std::make_unique<Noise3DTexture>(physicalDevice, logicalDevice, commandPool);

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
