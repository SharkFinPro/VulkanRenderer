#include "DotsPipeline.h"
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
                           const VkCommandPool& commandPool, const VkRenderPass& renderPass,
                           const VkExtent2D& swapChainExtent)
  : ComputePipeline(physicalDevice, logicalDevice), GraphicsPipeline(physicalDevice, logicalDevice), dotSpeed(1000.0f),
    previousTime(std::chrono::steady_clock::now())
{
  createUniforms();
  createShaderStorageBuffers(commandPool, swapChainExtent);

  createDescriptorSetLayouts();
  createDescriptorPool();
  createDescriptorSets();

  ComputePipeline::createPipeline();
  GraphicsPipeline::createPipeline(renderPass);
}

DotsPipeline::~DotsPipeline()
{
  vkDestroyDescriptorPool(ComputePipeline::logicalDevice->getDevice(), computeDescriptorPool, nullptr);

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

std::unique_ptr<VkPipelineColorBlendStateCreateInfo> DotsPipeline::defineColorBlendState()
{
  colorBlendAttachment = {
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  return std::make_unique<VkPipelineColorBlendStateCreateInfo>(colorBlendState);
}

std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> DotsPipeline::defineDepthStencilState()
{
  VkPipelineDepthStencilStateCreateInfo depthStencilState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_FALSE,
    .depthWriteEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineDepthStencilStateCreateInfo>(depthStencilState);
}

std::unique_ptr<VkPipelineDynamicStateCreateInfo> DotsPipeline::defineDynamicState()
{
  dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  return std::make_unique<VkPipelineDynamicStateCreateInfo>(dynamicState);
}

std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> DotsPipeline::defineInputAssemblyState()
{
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(inputAssemblyState);
}

std::unique_ptr<VkPipelineMultisampleStateCreateInfo> DotsPipeline::defineMultisampleState()
{
  VkPipelineMultisampleStateCreateInfo multisampleState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = GraphicsPipeline::physicalDevice->getMsaaSamples(),
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = VK_NULL_HANDLE,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineMultisampleStateCreateInfo>(multisampleState);
}

std::unique_ptr<VkPipelineRasterizationStateCreateInfo> DotsPipeline::defineRasterizationState()
{
  VkPipelineRasterizationStateCreateInfo rasterizationState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  return std::make_unique<VkPipelineRasterizationStateCreateInfo>(rasterizationState);
}

std::unique_ptr<VkPipelineVertexInputStateCreateInfo> DotsPipeline::defineVertexInputState()
{
  vertexBindingDescription = Particle::getBindingDescription();
  vertexAttributeDescriptions = Particle::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  return std::make_unique<VkPipelineVertexInputStateCreateInfo>(vertexInputState);
}

std::unique_ptr<VkPipelineViewportStateCreateInfo> DotsPipeline::defineViewportState()
{
  VkPipelineViewportStateCreateInfo viewportState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
  };

  return std::make_unique<VkPipelineViewportStateCreateInfo>(viewportState);
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

void DotsPipeline::createDescriptorPool()
{
  constexpr std::array<VkDescriptorPoolSize, 2> poolSizes {{
    {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    },
    {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2
    }
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  if (vkCreateDescriptorPool(ComputePipeline::logicalDevice->getDevice(), &poolCreateInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void DotsPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = computeDescriptorPool,
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