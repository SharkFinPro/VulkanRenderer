#include "ComputePipeline.h"

#include <utility>

ComputePipeline::ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{

}

ComputePipeline::~ComputePipeline()
{

}


void ComputePipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  ///
  // std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
  // layoutBindings[0].binding = 0;
  // layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // layoutBindings[0].descriptorCount = 1;
  // layoutBindings[0].pImmutableSamplers = nullptr;
  // layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  //
  // layoutBindings[1].binding = 1;
  // layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  // layoutBindings[1].descriptorCount = 1;
  // layoutBindings[1].pImmutableSamplers = nullptr;
  // layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  //
  // layoutBindings[2].binding = 2;
  // layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  // layoutBindings[2].descriptorCount = 1;
  // layoutBindings[2].pImmutableSamplers = nullptr;
  // layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  //
  // VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  // descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  // descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  // descriptorSetLayoutInfo.pBindings = layoutBindings.data();
  //
  // if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &descriptorSetLayoutInfo, nullptr,
  //                                 &computeDescriptorSetLayout) != VK_SUCCESS)
  // {
  //   throw std::runtime_error("failed to create descriptor set layout!");
  // }
  ///


  descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void ComputePipeline::createPipelineLayout()
{
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.data()
  };

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

}

void ComputePipeline::createPipeline()
{
  const ShaderModule computeShaderModule {
    logicalDevice->getDevice(),
    "assets/shaders/dots.comp.spv",
    VK_SHADER_STAGE_COMPUTE_BIT
  };

  const VkComputePipelineCreateInfo computePipelineCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = computeShaderModule.getShaderStageCreateInfo(),
    .layout = pipelineLayout
  };

  if (vkCreateComputePipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1,
                               &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}
