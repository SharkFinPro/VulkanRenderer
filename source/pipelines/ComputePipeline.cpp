#include "ComputePipeline.h"

#include <utility>

ComputePipeline::ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{

}

ComputePipeline::~ComputePipeline()
{
  vkDestroyPipeline(logicalDevice->getDevice(), pipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice->getDevice(), pipelineLayout, nullptr);
}

void ComputePipeline::createShader(const char* filename)
{
  shaderModule = std::make_unique<ShaderModule>(
    logicalDevice->getDevice(),
    filename,
    VK_SHADER_STAGE_COMPUTE_BIT
  );
}


void ComputePipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
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
  const VkComputePipelineCreateInfo computePipelineCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = shaderModule->getShaderStageCreateInfo(),
    .layout = pipelineLayout
  };

  if (vkCreateComputePipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1,
                               &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute pipeline!");
  }

  shaderModule.reset();
}
