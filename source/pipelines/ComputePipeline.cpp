#include "ComputePipeline.h"
#include "ShaderModule.h"
#include "../utilities/Buffers.h"

#include <cmath>
#include <stdexcept>
#include <random>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

ComputePipeline::ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                                 std::shared_ptr<LogicalDevice> logicalDevice,
                                 VkCommandPool& commandPool)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{
  createComputePipeline();

  createUniformBuffers();
  createShaderStorageBuffers(commandPool);

  createDescriptorPool();
  createDescriptorSets();
}

ComputePipeline::~ComputePipeline() {
  vkDestroyDescriptorPool(logicalDevice->getDevice(), computeDescriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), computeDescriptorSetLayout,
                               nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroyBuffer(logicalDevice->getDevice(), shaderStorageBuffers[i], nullptr);
    vkFreeMemory(logicalDevice->getDevice(), shaderStorageBuffersMemory[i], nullptr);

    vkDestroyBuffer(logicalDevice->getDevice(), uniformBuffers[i], nullptr);
    vkFreeMemory(logicalDevice->getDevice(), uniformBuffersMemory[i], nullptr);
  }

  vkDestroyPipeline(logicalDevice->getDevice(), computePipeline, nullptr);

  vkDestroyPipelineLayout(logicalDevice->getDevice(), computePipelineLayout, nullptr);
}

void ComputePipeline::compute(VkCommandBuffer &commandBuffer,
                              uint32_t currentFrame)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame],
                          0, nullptr);

  vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
}

void ComputePipeline::render(VkCommandBuffer &commandBuffer,
                             uint32_t currentFrame)
{
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                         &shaderStorageBuffers[currentFrame], offsets);

  vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void ComputePipeline::updateUniformBuffer(uint32_t currentFrame) const
{
  UniformBufferObject ubo{};
  ubo.deltaTime = lastFrameTime * 2.0f;

  memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void ComputePipeline::createComputePipeline()
{
  ShaderModule vertexShaderModule{logicalDevice->getDevice(),
                                  "assets/shaders/compute.vert.spv",
                                  VK_SHADER_STAGE_VERTEX_BIT};
  ShaderModule fragmentShaderModule{logicalDevice->getDevice(),
                                    "assets/shaders/compute.frag.spv",
                                    VK_SHADER_STAGE_FRAGMENT_BIT};
  ShaderModule computeShaderModule{logicalDevice->getDevice(),
                                   "assets/shaders/compute.comp.spv",
                                   VK_SHADER_STAGE_COMPUTE_BIT};

  std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
  layoutBindings[0].binding = 0;
  layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[0].descriptorCount = 1;
  layoutBindings[0].pImmutableSamplers = nullptr;
  layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  layoutBindings[1].binding = 1;
  layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBindings[1].descriptorCount = 1;
  layoutBindings[1].pImmutableSamplers = nullptr;
  layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  layoutBindings[2].binding = 2;
  layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBindings[2].descriptorCount = 1;
  layoutBindings[2].pImmutableSamplers = nullptr;
  layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  descriptorSetLayoutInfo.pBindings = layoutBindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(),
                                  &descriptorSetLayoutInfo, nullptr,
                                  &computeDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo,
                             nullptr, &computePipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = computePipelineLayout;
  computePipelineCreateInfo.stage = computeShaderModule.getShaderStageCreateInfo();

  if (vkCreateComputePipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1,
                               &computePipelineCreateInfo, nullptr,
                               &computePipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

void ComputePipeline::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::createBuffer(logicalDevice->getDevice(),
                          physicalDevice->getPhysicalDevice(), bufferSize,
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(logicalDevice->getDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
  }
}

void ComputePipeline::createShaderStorageBuffers(VkCommandPool& commandPool)
{
  shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto& particle : particles)
  {
    const float r = 0.25f * sqrt(distribution(randomEngine));
    const float theta = distribution(randomEngine) * 2 * M_PI;
    const float x = r * std::cos(theta) * HEIGHT / WIDTH;
    const float y = r * std::sin(theta);
    particle.position = glm::vec2(x, y);
    particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    particle.color =
        glm::vec4(distribution(randomEngine), distribution(randomEngine),
                  distribution(randomEngine), 1.0f);
  }

  constexpr VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(logicalDevice->getDevice(),
                        physicalDevice->getPhysicalDevice(), bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0,
              &data);
  memcpy(data, particles.data(), bufferSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    Buffers::createBuffer(
        logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i],
        shaderStorageBuffersMemory[i]);

    Buffers::copyBuffer(logicalDevice->getDevice(), commandPool,
      logicalDevice->getComputeQueue(), stagingBuffer,
      shaderStorageBuffers[i], bufferSize);
  }

  vkDestroyBuffer(logicalDevice->getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), stagingBufferMemory, nullptr);
}

void ComputePipeline::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;


  VkDescriptorPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolCreateInfo.pPoolSizes = poolSizes.data();
  poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void ComputePipeline::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = computeDescriptorPool;
  allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocateInfo.pSetLayouts = layouts.data();

  computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo, computeDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffers[i];
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = computeDescriptorSets[i];
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].pBufferInfo = &uniformBufferInfo;

    VkDescriptorBufferInfo storageBufferInfoLastFrame{};
    storageBufferInfoLastFrame.buffer = shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
    storageBufferInfoLastFrame.offset = 0;
    storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstSet = computeDescriptorSets[i];
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].pBufferInfo = &storageBufferInfoLastFrame;

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
    storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[i];
    storageBufferInfoCurrentFrame.offset = 0;
    storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[2].dstSet = computeDescriptorSets[i];
    writeDescriptorSets[2].dstBinding = 2;
    writeDescriptorSets[2].dstArrayElement = 0;
    writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[2].descriptorCount = 1;
    writeDescriptorSets[2].pBufferInfo = &storageBufferInfoCurrentFrame;

    vkUpdateDescriptorSets(logicalDevice->getDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
  }
}