#include "ComputePipeline.h"
#include "ShaderModule.h"
#include "../utilities/Buffers.h"

#include <array>
#include <stdexcept>
#include <random>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

ComputePipeline::ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                                 std::shared_ptr<LogicalDevice> logicalDevice)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{
  createPipeline();

  createDescriptorPool();
  createDescriptorSets();

  initializeParticles();
}

ComputePipeline::~ComputePipeline() {}

void ComputePipeline::render(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  // VkDeviceSize offsets[] = {0};
  // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], offsets);
  //
  // const int PARTICLE_COUNT = 1000;
  // vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void ComputePipeline::createPipeline() {
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
  descriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount =
      static_cast<uint32_t>(layoutBindings.size());
  descriptorSetLayoutInfo.pBindings = layoutBindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(),
                                  &descriptorSetLayoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo,
                             nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage =
      computeShaderModule.getShaderStageCreateInfo();

  if (vkCreateComputePipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1,
                               &computePipelineCreateInfo, nullptr,
                               &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

void ComputePipeline::initializeParticles() {
  shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

  std::default_random_engine randomEngine((unsigned int)time(nullptr));
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto& particle : particles) {
    float r = 0.25f * sqrt(distribution(randomEngine));
    float theta = distribution(randomEngine) * 2 * M_PI;
    float x = r * cos(theta) * HEIGHT / WIDTH;
    float y = r * sin(theta);
    particle.position = glm::vec2(x, y);
    particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    particle.color = glm::vec4(distribution(randomEngine),
                               distribution(randomEngine),
                               distribution(randomEngine), 1.0f);
  }

  VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

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

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    Buffers::createBuffer(
        logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i],
        shaderStorageBuffersMemory[i]);
    // TODO: Copy Buffer
    // Buffers::copyBuffer()
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
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      uniformBuffers[i], uniformBuffersMemory[i]);
  }
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

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void ComputePipeline::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocateInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
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
    writeDescriptorSets[0].dstSet = descriptorSets[i];
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
    writeDescriptorSets[1].dstSet = descriptorSets[i];
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].pBufferInfo = &storageBufferInfoLastFrame;

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
    storageBufferInfoLastFrame.buffer = shaderStorageBuffers[i];
    storageBufferInfoLastFrame.offset = 0;
    storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[2].dstSet = descriptorSets[i];
    writeDescriptorSets[2].dstBinding = 2;
    writeDescriptorSets[2].dstArrayElement = 0;
    writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[2].descriptorCount = 1;
    writeDescriptorSets[2].pBufferInfo = &storageBufferInfoCurrentFrame;

    vkUpdateDescriptorSets(logicalDevice->getDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
  }
}