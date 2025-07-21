#include "SmokePipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "config/Uniforms.h"
#include "vertexInputs/SmokeParticle.h"
#include "../../core/logicalDevice/LogicalDevice.h"
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

SmokePipeline::SmokePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const VkCommandPool& commandPool,
                             const VkRenderPass& renderPass,
                             const VkDescriptorPool descriptorPool,
                             const glm::vec3 position,
                             const uint32_t numParticles)
  : ComputePipeline(logicalDevice), GraphicsPipeline(logicalDevice),
    m_descriptorPool(descriptorPool), m_dotSpeed(0.75f), m_previousTime(std::chrono::steady_clock::now()),
    m_numParticles(numParticles)
{
  m_smokeUBO.systemPosition = position;

  createUniforms();
  createShaderStorageBuffers(commandPool);

  createDescriptorSetLayouts();

  createDescriptorSets();

  ComputePipeline::createPipeline();
  GraphicsPipeline::createPipeline(renderPass);
}

SmokePipeline::~SmokePipeline()
{
  ComputePipeline::m_logicalDevice->destroyDescriptorSetLayout(m_computeDescriptorSetLayout);

  for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::destroyBuffer(ComputePipeline::m_logicalDevice, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);
  }
}

void SmokePipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer, const uint32_t currentFrame) const
{
  commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipeline);

  commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipelineLayout, 0,
                                    1, &m_computeDescriptorSets[currentFrame]);

  commandBuffer->dispatch(m_numParticles / 256, 1, 1);
}

void SmokePipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, objects);

  constexpr VkDeviceSize offsets[] = {0};
  renderInfo->commandBuffer->bindVertexBuffers(0, 1, &m_shaderStorageBuffers[renderInfo->currentFrame], offsets);

  renderInfo->commandBuffer->draw(m_numParticles, 1, 0, 0);
}

void SmokePipeline::displayGui()
{
  ImGui::SliderFloat3("Position", &m_smokeUBO.systemPosition[0], -20, 20);

  ImGui::SliderFloat("Speed", &m_dotSpeed, 0.001f, 10.0f);

  ImGui::SliderFloat("Spread Factor", &m_smokeUBO.spreadFactor, 0.0f, 3.0f);

  ImGui::SliderFloat("Max Spread Distance", &m_smokeUBO.maxSpreadDistance, 0.0f, 20.0f);

  ImGui::SliderFloat("Wind Strength", &m_smokeUBO.windStrength, 0.0f, 3.0f);
}

void SmokePipeline::loadComputeShaders()
{
  ComputePipeline::createShader("assets/shaders/Smoke.comp.spv");
}

void SmokePipeline::loadComputeDescriptorSetLayouts()
{
  ComputePipeline::loadDescriptorSetLayout(m_computeDescriptorSetLayout);
}

void SmokePipeline::loadGraphicsDescriptorSetLayouts()
{
  GraphicsPipeline::loadDescriptorSetLayout(m_computeDescriptorSetLayout);
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
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(GraphicsPipeline::m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateSmokeParticle);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void SmokePipeline::createUniforms()
{
  m_deltaTimeUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(DeltaTimeUniform));

  m_transformUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(ViewProjTransformUniform));

  m_smokeUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(SmokeUniform));

  m_lightMetadataUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(LightMetadataUniform));

  m_lightsUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(LightUniform));
}

void SmokePipeline::createShaderStorageBuffers(const VkCommandPool& commandPool)
{
  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> colorDistribution(0.25f, 1.0f);
  std::uniform_real_distribution<float> largeDistribution(-1000.0f, 1000.0f);

  std::vector<SmokeParticle> particles(m_numParticles);

  double currentTTL = 0;
  const double ttlSpan = 8.0 / static_cast<double>(m_numParticles) * 1.5;

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
  m_shaderStorageBuffers.resize(ComputePipeline::m_logicalDevice->getMaxFramesInFlight());
  m_shaderStorageBuffersMemory.resize(ComputePipeline::m_logicalDevice->getMaxFramesInFlight());

  const VkDeviceSize bufferSize = sizeof(SmokeParticle) * m_numParticles;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(ComputePipeline::m_logicalDevice, bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  ComputePipeline::m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [particles, bufferSize](void* data) {
    memcpy(data, particles.data(), bufferSize);
  });

  for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::createBuffer(ComputePipeline::m_logicalDevice, bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);

    Buffers::copyBuffer(ComputePipeline::m_logicalDevice, commandPool, ComputePipeline::m_logicalDevice->getComputeQueue(),
                        stagingBuffer, m_shaderStorageBuffers[i], bufferSize);
  }

  Buffers::destroyBuffer(ComputePipeline::m_logicalDevice, stagingBuffer, stagingBufferMemory);
}

void SmokePipeline::createDescriptorSetLayouts()
{
  const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
    .pBindings = layoutBindings.data()
  };

  m_computeDescriptorSetLayout = ComputePipeline::m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutInfo);
}

void SmokePipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(ComputePipeline::m_logicalDevice->getMaxFramesInFlight(), m_computeDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = m_descriptorPool,
    .descriptorSetCount = ComputePipeline::m_logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  m_computeDescriptorSets.resize(ComputePipeline::m_logicalDevice->getMaxFramesInFlight());
  ComputePipeline::m_logicalDevice->allocateDescriptorSets(allocateInfo, m_computeDescriptorSets.data());

  for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    createDescriptorSet(i);

    constexpr DeltaTimeUniform deltaTimeUBO{0};

    m_deltaTimeUniform->update(i, &deltaTimeUBO);
    m_smokeUniform->update(i, &m_smokeUBO);
  }
}

void SmokePipeline::createDescriptorSet(const uint32_t set) const
{
  const VkDescriptorBufferInfo storageBufferInfoLastFrame {
    .buffer = m_shaderStorageBuffers[(set - 1) % ComputePipeline::m_logicalDevice->getMaxFramesInFlight()],
    .offset = 0,
    .range = sizeof(SmokeParticle) * m_numParticles
  };

  const VkDescriptorBufferInfo storageBufferInfoCurrentFrame {
    .buffer = m_shaderStorageBuffers[set],
    .offset = 0,
    .range = sizeof(SmokeParticle) * m_numParticles
  };

  const std::array<VkWriteDescriptorSet, 6> writeDescriptorSets {{
    m_deltaTimeUniform->getDescriptorSet(0, m_computeDescriptorSets[set], set),
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = m_computeDescriptorSets[set],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &storageBufferInfoLastFrame
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = m_computeDescriptorSets[set],
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &storageBufferInfoCurrentFrame
    },
    m_transformUniform->getDescriptorSet(3, m_computeDescriptorSets[set], set),
    m_smokeUniform->getDescriptorSet(4, m_computeDescriptorSets[set], set),
    m_lightMetadataUniform->getDescriptorSet(5, m_computeDescriptorSets[set], set)
  }};

  ComputePipeline::m_logicalDevice->updateDescriptorSets(writeDescriptorSets.size(),
                                          writeDescriptorSets.data());
}

void SmokePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  if (!m_ran)
  {
    m_previousTime = std::chrono::steady_clock::now();
    m_ran = true;
  }

  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - m_previousTime).count();
  m_previousTime = currentTime;

  const DeltaTimeUniform deltaTimeUBO{m_dotSpeed * dt};

  m_deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);

  const ViewProjTransformUniform transformUBO {
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->getProjectionMatrix()
  };

  m_transformUniform->update(renderInfo->currentFrame, &transformUBO);

  m_smokeUniform->update(renderInfo->currentFrame, &m_smokeUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);
}

void SmokePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, ComputePipeline::m_pipelineLayout, 0,
                                                1, &m_computeDescriptorSets[renderInfo->currentFrame]);
}

void SmokePipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (m_prevNumLights != lights.size())
  {
    ComputePipeline::m_logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    m_lightsUniform.reset();

    m_lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    m_lightsUniform = std::make_shared<UniformBuffer>(ComputePipeline::m_logicalDevice, m_lightsUniformBufferSize);

    for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
    {
      m_lightMetadataUniform->update(i, &lightMetadataUBO);

      auto descriptorSet = m_lightsUniform->getDescriptorSet(6, m_computeDescriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      ComputePipeline::m_logicalDevice->updateDescriptorSets(1, &descriptorSet);
    }

    m_prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  m_lightsUniform->update(currentFrame, lightUniforms.data());
}
