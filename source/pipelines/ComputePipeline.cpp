#include "ComputePipeline.h"
#include "ShaderModule.h"

#include <array>
#include <stdexcept>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

ComputePipeline::ComputePipeline()
{
  shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
}

ComputePipeline::~ComputePipeline() {}

void ComputePipeline::render(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
{
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], offsets);

  const int PARTICLE_COUNT = 1000;
  vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void ComputePipeline::createPipeline()
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

  layoutBindings[1].binding = 0;
  layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[1].descriptorCount = 1;
  layoutBindings[1].pImmutableSamplers = nullptr;
  layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  layoutBindings[2].binding = 0;
  layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[2].descriptorCount = 1;
  layoutBindings[2].pImmutableSamplers = nullptr;
  layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  descriptorSetLayoutInfo.pBindings = layoutBindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage = computeShaderModule.getShaderStageCreateInfo();

  if (vkCreateComputePipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}