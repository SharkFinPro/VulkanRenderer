#include "DotsPipeline.h"
#include "GraphicsPipelineStates.h"
#include "../Particle.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../utilities/Buffers.h"
#include <cmath>
#include <stdexcept>
#include <random>
#include <cstring>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

DotsPipeline::DotsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const VkCommandPool& commandPool,
                           const VkRenderPass& renderPass,
                           const VkExtent2D& swapChainExtent,
                           const VkDescriptorPool descriptorPool)
  : ComputePipeline(physicalDevice, logicalDevice), GraphicsPipeline(physicalDevice, logicalDevice),
    descriptorPool(descriptorPool), dotSpeed(1000.0f), previousTime(std::chrono::steady_clock::now())
{
  createUniforms();
  createShaderStorageBuffers(commandPool, swapChainExtent);

  createDescriptorSetLayouts();

  createDescriptorSets();

  ComputePipeline::createPipeline();
  GraphicsPipeline::createPipeline(renderPass);
}

DotsPipeline::~DotsPipeline()
{
  vkDestroyDescriptorSetLayout(ComputePipeline::logicalDevice->getDevice(), computeDescriptorSetLayout, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::destroyBuffer(ComputePipeline::logicalDevice, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
  }
}

void DotsPipeline::compute(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame) const
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          ComputePipeline::pipelineLayout, 0, 1, &computeDescriptorSets[currentFrame],
                          0, nullptr);

  vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
}

void DotsPipeline::render(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame, const VkExtent2D swapChainExtent)
{
  updateUniformBuffer(currentFrame);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline::pipeline);

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

  constexpr VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], offsets);

  vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void DotsPipeline::loadComputeShaders()
{
  ComputePipeline::createShader("assets/shaders/dots.comp.spv");
}

void DotsPipeline::loadComputeDescriptorSetLayouts()
{
  ComputePipeline::loadDescriptorSetLayout(computeDescriptorSetLayout);
}

void DotsPipeline::loadGraphicsShaders()
{
  GraphicsPipeline::createShader("assets/shaders/dots.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  GraphicsPipeline::createShader("assets/shaders/dots.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void DotsPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateDots);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilStateNone);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStatePointList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(GraphicsPipeline::physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateParticle);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void DotsPipeline::updateUniformBuffer(const uint32_t currentFrame)
{
  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - previousTime).count();
  previousTime = currentTime;

  const DeltaTimeUniform deltaTimeUBO{dotSpeed * dt};

  deltaTimeUniform->update(currentFrame, &deltaTimeUBO, sizeof(DeltaTimeUniform));
}

void DotsPipeline::createUniforms()
{
  deltaTimeUniform = std::make_unique<UniformBuffer>(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice,
                                                     MAX_FRAMES_IN_FLIGHT, sizeof(DeltaTimeUniform));
}

void DotsPipeline::createShaderStorageBuffers(const VkCommandPool& commandPool, const VkExtent2D& swapChainExtent)
{
  shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::uniform_real_distribution<float> largeDistribution(-4.0f, 4.0f);

  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto&[position, velocity, color] : particles)
  {
    const float r = sqrtf(distribution(randomEngine)) * 0.25f;
    const float theta = distribution(randomEngine) * 2.0f * 3.14159265358979323846f;
    const float x = r * std::cos(theta) * static_cast<float>(swapChainExtent.height) / static_cast<float>(swapChainExtent.width);
    const float y = r * std::sin(theta);
    position = glm::vec2(x * largeDistribution(randomEngine), y * largeDistribution(randomEngine));
    velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    color = glm::vec4(distribution(randomEngine), distribution(randomEngine),
                               distribution(randomEngine), 1.0f);
  }

  constexpr VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

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

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::createBuffer(ComputePipeline::logicalDevice, ComputePipeline::physicalDevice, bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);

    Buffers::copyBuffer(ComputePipeline::logicalDevice, commandPool, ComputePipeline::logicalDevice->getComputeQueue(),
                        stagingBuffer, shaderStorageBuffers[i], bufferSize);
  }

  Buffers::destroyBuffer(ComputePipeline::logicalDevice, stagingBuffer, stagingBufferMemory);
}

void DotsPipeline::createDescriptorSetLayouts()
{
  constexpr std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings {{
    {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    },
    {
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    }
  }};

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

void DotsPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .pSetLayouts = layouts.data()
  };

  computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(ComputePipeline::logicalDevice->getDevice(), &allocateInfo, computeDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    const VkDescriptorBufferInfo storageBufferInfoLastFrame {
      .buffer = shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT],
      .offset = 0,
      .range = sizeof(Particle) * PARTICLE_COUNT
    };

    const VkDescriptorBufferInfo storageBufferInfoCurrentFrame {
      .buffer = shaderStorageBuffers[i],
      .offset = 0,
      .range = sizeof(Particle) * PARTICLE_COUNT
    };

    std::array<VkWriteDescriptorSet, 3> writeDescriptorSets {{
      deltaTimeUniform->getDescriptorSet(0, computeDescriptorSets[i], i),
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = computeDescriptorSets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &storageBufferInfoLastFrame
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = computeDescriptorSets[i],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &storageBufferInfoCurrentFrame
      }
    }};

    vkUpdateDescriptorSets(ComputePipeline::logicalDevice->getDevice(), writeDescriptorSets.size(),
                           writeDescriptorSets.data(), 0, nullptr);
  }
}