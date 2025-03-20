#include "SmokePipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../SmokeParticle.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include "../../utilities/Buffers.h"
#include <imgui.h>
#include <cmath>
#include <stdexcept>
#include <random>
#include <cstring>

constexpr std::array<VkDescriptorSetLayoutBinding, 7> layoutBindings {{
  { // DT
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
  },
  { // Last Frame SB
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  },
  { // Current Frame SB
    .binding = 2,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  },
  { // Transform
    .binding = 3,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  },
  { // Smoke
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  },
  { // Light Metadata
    .binding = 5,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  },
  { // Lights
    .binding = 6,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  }
}};

SmokePipeline::SmokePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const VkCommandPool& commandPool,
                             const VkRenderPass& renderPass,
                             const VkDescriptorPool descriptorPool,
                             const glm::vec3 position,
                             const uint32_t numParticles)
  : ComputePipeline(physicalDevice, logicalDevice), GraphicsPipeline(physicalDevice, logicalDevice),
    descriptorPool(descriptorPool), dotSpeed(0.75f), previousTime(std::chrono::steady_clock::now()),
    numParticles(numParticles)
{
  smokeUBO.systemPosition = position;

  createUniforms();
  createShaderStorageBuffers(commandPool);

  createDescriptorSetLayouts();

  createDescriptorSets();

  ComputePipeline::createPipeline();
  GraphicsPipeline::createPipeline(renderPass);
}

SmokePipeline::~SmokePipeline()
{
  vkDestroyDescriptorSetLayout(ComputePipeline::logicalDevice->getDevice(), computeDescriptorSetLayout, nullptr);

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::destroyBuffer(ComputePipeline::logicalDevice, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
  }
}

void SmokePipeline::compute(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame) const
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          ComputePipeline::pipelineLayout, 0, 1, &computeDescriptorSets[currentFrame],
                          0, nullptr);

  vkCmdDispatch(commandBuffer, numParticles / 256, 1, 1);
}

void SmokePipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, objects);

  constexpr VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(renderInfo->commandBuffer, 0, 1, &shaderStorageBuffers[renderInfo->currentFrame], offsets);

  vkCmdDraw(renderInfo->commandBuffer, numParticles, 1, 0, 0);
}

void SmokePipeline::displayGui()
{
  ImGui::Begin("Smoke");

  ImGui::SliderFloat("Speed", &dotSpeed, 0.001f, 10.0f);

  ImGui::SliderFloat("Spread Factor", &smokeUBO.spreadFactor, 0.0f, 3.0f);

  ImGui::SliderFloat("Max Spread Distance", &smokeUBO.maxSpreadDistance, 0.0f, 20.0f);

  ImGui::SliderFloat("Wind Strength", &smokeUBO.windStrength, 0.0f, 3.0f);

  ImGui::End();
}

void SmokePipeline::loadComputeShaders()
{
  ComputePipeline::createShader("assets/shaders/Smoke.comp.spv");
}

void SmokePipeline::loadComputeDescriptorSetLayouts()
{
  ComputePipeline::loadDescriptorSetLayout(computeDescriptorSetLayout);
}

void SmokePipeline::loadGraphicsDescriptorSetLayouts()
{
  GraphicsPipeline::loadDescriptorSetLayout(computeDescriptorSetLayout);
}

void SmokePipeline::loadGraphicsShaders()
{
  GraphicsPipeline::createShader("assets/shaders/Smoke.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  GraphicsPipeline::createShader("assets/shaders/Smoke.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void SmokePipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateSmoke);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStatePointList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(GraphicsPipeline::physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateSmokeParticle);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void SmokePipeline::createUniforms()
{
  deltaTimeUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice,
                                                       sizeof(DeltaTimeUniform));

  transformUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice,
                                                     sizeof(TransformUniform));

  smokeUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice,
                                                 sizeof(SmokeUniform));

  lightMetadataUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, sizeof(LightUniform));
}

void SmokePipeline::createShaderStorageBuffers(const VkCommandPool& commandPool)
{
  shaderStorageBuffers.resize(ComputePipeline::logicalDevice->getMaxFramesInFlight());
  shaderStorageBuffersMemory.resize(ComputePipeline::logicalDevice->getMaxFramesInFlight());

  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> colorDistribution(0.25f, 1.0f);

  std::vector<SmokeParticle> particles(numParticles);

  float currentTTL = 0;
  const float ttlSpan = 8.0f / static_cast<float>(numParticles) * 1.5f;

  for (auto& [positionTtl, _1, _2, color] : particles)
  {
    color = glm::vec4(glm::vec3(colorDistribution(randomEngine)), 0);

    positionTtl = glm::vec4(color.x * 1000.0f, 0, 0, currentTTL);

    currentTTL -= currentTTL < 4.0f ? ttlSpan * 4.0f : ttlSpan;
  }

  uploadShaderStorageBuffers(commandPool, particles);
}

void SmokePipeline::uploadShaderStorageBuffers(const VkCommandPool& commandPool,
                                               const std::vector<SmokeParticle>& particles)
{
  const VkDeviceSize bufferSize = sizeof(SmokeParticle) * numParticles;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(ComputePipeline::logicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, particles.data(), bufferSize);
  vkUnmapMemory(ComputePipeline::logicalDevice->getDevice(), stagingBufferMemory);

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::createBuffer(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);

    Buffers::copyBuffer(ComputePipeline::logicalDevice, commandPool, ComputePipeline::logicalDevice->getComputeQueue(),
                        stagingBuffer, shaderStorageBuffers[i], bufferSize);
  }

  Buffers::destroyBuffer(ComputePipeline::logicalDevice, stagingBuffer, stagingBufferMemory);
}

void SmokePipeline::createDescriptorSetLayouts()
{
  const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
    .pBindings = layoutBindings.data()
  };

  if (vkCreateDescriptorSetLayout(ComputePipeline::logicalDevice->getDevice(), &descriptorSetLayoutInfo, nullptr,
                                  &computeDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void SmokePipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(ComputePipeline::logicalDevice->getMaxFramesInFlight(), computeDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = ComputePipeline::logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  computeDescriptorSets.resize(ComputePipeline::logicalDevice->getMaxFramesInFlight());
  if (vkAllocateDescriptorSets(ComputePipeline::logicalDevice->getDevice(), &allocateInfo, computeDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    createDescriptorSet(i);
  }
}

void SmokePipeline::createDescriptorSet(const uint32_t set) const
{
  const VkDescriptorBufferInfo storageBufferInfoLastFrame {
    .buffer = shaderStorageBuffers[(set - 1) % ComputePipeline::logicalDevice->getMaxFramesInFlight()],
    .offset = 0,
    .range = sizeof(SmokeParticle) * numParticles
  };

  const VkDescriptorBufferInfo storageBufferInfoCurrentFrame {
    .buffer = shaderStorageBuffers[set],
    .offset = 0,
    .range = sizeof(SmokeParticle) * numParticles
  };

  std::array<VkWriteDescriptorSet, 6> writeDescriptorSets {{
    deltaTimeUniform->getDescriptorSet(0, computeDescriptorSets[set], set),
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = computeDescriptorSets[set],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &storageBufferInfoLastFrame
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = computeDescriptorSets[set],
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &storageBufferInfoCurrentFrame
    },
    transformUniform->getDescriptorSet(3, computeDescriptorSets[set], set),
    smokeUniform->getDescriptorSet(4, computeDescriptorSets[set], set),
    lightMetadataUniform->getDescriptorSet(5, computeDescriptorSets[set], set)
  }};

  vkUpdateDescriptorSets(ComputePipeline::logicalDevice->getDevice(), writeDescriptorSets.size(),
                         writeDescriptorSets.data(), 0, nullptr);
}

void SmokePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  if (!ran)
  {
    previousTime = std::chrono::steady_clock::now();
    ran = true;
  }

  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - previousTime).count();
  previousTime = currentTime;

  const DeltaTimeUniform deltaTimeUBO{dotSpeed * dt};

  deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO, sizeof(DeltaTimeUniform));

  constexpr glm::vec3 position{0, 0, 0};
  constexpr glm::vec3 scale{1};
  constexpr glm::quat orientation{1, 0, 0, 0};

  const TransformUniform transformUBO {
    .model = glm::translate(glm::mat4(1.0f), position)
                            * glm::mat4(orientation)
                            * glm::scale(glm::mat4(1.0f), scale),
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->getProjectionMatrix()
  };

  transformUniform->update(renderInfo->currentFrame, &transformUBO, sizeof(TransformUniform));

  smokeUniform->update(renderInfo->currentFrame, &smokeUBO, sizeof(SmokeUniform));

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);
}

void SmokePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  vkCmdBindDescriptorSets(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ComputePipeline::pipelineLayout, 0, 1,
                          &computeDescriptorSets[renderInfo->currentFrame], 0, nullptr);
}

void SmokePipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (prevNumLights != lights.size())
  {
    ComputePipeline::logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    lightsUniform.reset();

    lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    lightsUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, lightsUniformBufferSize);

    for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
    {
      lightMetadataUniform->update(i, &lightMetadataUBO, sizeof(lightMetadataUBO));

      auto descriptorSet = lightsUniform->getDescriptorSet(6, computeDescriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      vkUpdateDescriptorSets(ComputePipeline::logicalDevice->getDevice(), 1, &descriptorSet, 0, nullptr);
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
