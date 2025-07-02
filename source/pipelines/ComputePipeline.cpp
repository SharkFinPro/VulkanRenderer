#include "ComputePipeline.h"
#include "ShaderModule.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include <stdexcept>

ComputePipeline::ComputePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                 const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Pipeline(physicalDevice, logicalDevice)
{}

void ComputePipeline::createShader(const char* filename)
{
  shaderModule = std::make_unique<ShaderModule>(logicalDevice, filename, VK_SHADER_STAGE_COMPUTE_BIT);
}

void ComputePipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void ComputePipeline::createPipelineLayout()
{
  loadComputeDescriptorSetLayouts();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.data()
  };

  pipelineLayout = logicalDevice->createPipelineLayout(pipelineLayoutInfo);
}

void ComputePipeline::createPipeline()
{
  createPipelineLayout();

  loadComputeShaders();

  const VkComputePipelineCreateInfo computePipelineCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = shaderModule->getShaderStageCreateInfo(),
    .layout = pipelineLayout
  };

  pipeline = logicalDevice->createPipeline(computePipelineCreateInfo);

  shaderModule.reset();
}
