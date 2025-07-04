#include "SmokePipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../SmokeParticle.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include "../../utilities/Buffers.h"
#include <imgui.h>
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
  ComputePipeline::logicalDevice->destroyDescriptorSetLayout(computeDescriptorSetLayout);

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::destroyBuffer(ComputePipeline::logicalDevice, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
  }
}

void SmokePipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer, const uint32_t currentFrame) const
{
  commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::pipeline);

  commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::pipelineLayout, 0,
                                    1, &computeDescriptorSets[currentFrame]);

  commandBuffer->dispatch(numParticles / 256, 1, 1);
}

void SmokePipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, objects);

  constexpr VkDeviceSize offsets[] = {0};
  renderInfo->commandBuffer->bindVertexBuffers(0, 1, &shaderStorageBuffers[renderInfo->currentFrame], offsets);

  renderInfo->commandBuffer->draw(numParticles, 1, 0, 0);
}

void SmokePipeline::displayGui()
{
  ImGui::SliderFloat3("Position", &smokeUBO.systemPosition[0], -20, 20);

  ImGui::SliderFloat("Speed", &dotSpeed, 0.001f, 10.0f);

  ImGui::SliderFloat("Spread Factor", &smokeUBO.spreadFactor, 0.0f, 3.0f);

  ImGui::SliderFloat("Max Spread Distance", &smokeUBO.maxSpreadDistance, 0.0f, 20.0f);

  ImGui::SliderFloat("Wind Strength", &smokeUBO.windStrength, 0.0f, 3.0f);
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
  deltaTimeUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, sizeof(DeltaTimeUniform));

  transformUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, sizeof(ViewProjTransformUniform));

  smokeUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, sizeof(SmokeUniform));

  lightMetadataUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, sizeof(LightUniform));
}

void SmokePipeline::createShaderStorageBuffers(const VkCommandPool& commandPool)
{
  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> colorDistribution(0.25f, 1.0f);
  std::uniform_real_distribution<float> largeDistribution(-1000.0f, 1000.0f);

  std::vector<SmokeParticle> particles(numParticles);

  double currentTTL = 0;
  const double ttlSpan = 8.0 / static_cast<double>(numParticles) * 1.5;

  for (auto& [positionTtl, velocityColor] : particles)
  {
    velocityColor.w = colorDistribution(randomEngine);

    positionTtl = glm::vec4(largeDistribution(randomEngine), 0, 0, currentTTL);

    currentTTL -= currentTTL > -4.0f ? ttlSpan * 4.0f : ttlSpan;
  }

  uploadShaderStorageBuffers(commandPool, particles);
}

void SmokePipeline::uploadShaderStorageBuffers(const VkCommandPool& commandPool,
                                               const std::vector<SmokeParticle>& particles)
{
  shaderStorageBuffers.resize(ComputePipeline::logicalDevice->getMaxFramesInFlight());
  shaderStorageBuffersMemory.resize(ComputePipeline::logicalDevice->getMaxFramesInFlight());

  const VkDeviceSize bufferSize = sizeof(SmokeParticle) * numParticles;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(ComputePipeline::logicalDevice, bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  ComputePipeline::logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [particles, bufferSize](void* data) {
    memcpy(data, particles.data(), bufferSize);
  });

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::createBuffer(ComputePipeline::logicalDevice, bufferSize,
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

  computeDescriptorSetLayout = ComputePipeline::logicalDevice->createDescriptorSetLayout(descriptorSetLayoutInfo);
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
  ComputePipeline::logicalDevice->allocateDescriptorSets(allocateInfo, computeDescriptorSets.data());

  for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
  {
    createDescriptorSet(i);

    constexpr DeltaTimeUniform deltaTimeUBO{0};

    deltaTimeUniform->update(i, &deltaTimeUBO);
    smokeUniform->update(i, &smokeUBO);
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

  const std::array<VkWriteDescriptorSet, 6> writeDescriptorSets {{
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

  ComputePipeline::logicalDevice->updateDescriptorSets(writeDescriptorSets.size(),
                                          writeDescriptorSets.data());
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

  deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);

  const ViewProjTransformUniform transformUBO {
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->getProjectionMatrix()
  };

  transformUniform->update(renderInfo->currentFrame, &transformUBO);

  smokeUniform->update(renderInfo->currentFrame, &smokeUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);
}

void SmokePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, ComputePipeline::pipelineLayout, 0,
                                                1, &computeDescriptorSets[renderInfo->currentFrame]);
}

void SmokePipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
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

    lightsUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, lightsUniformBufferSize);

    for (size_t i = 0; i < ComputePipeline::logicalDevice->getMaxFramesInFlight(); i++)
    {
      lightMetadataUniform->update(i, &lightMetadataUBO);

      auto descriptorSet = lightsUniform->getDescriptorSet(6, computeDescriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      ComputePipeline::logicalDevice->updateDescriptorSets(1, &descriptorSet);
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
