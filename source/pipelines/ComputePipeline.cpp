#include "ComputePipeline.h"
#include "ShaderModule.h"
#include "../components/core/logicalDevice/LogicalDevice.h"

ComputePipeline::ComputePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Pipeline(logicalDevice)
{}

void ComputePipeline::createShader(const char* filename)
{
  m_shaderModule = std::make_unique<ShaderModule>(m_logicalDevice, filename, VK_SHADER_STAGE_COMPUTE_BIT);
}

void ComputePipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  m_descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void ComputePipeline::createPipelineLayout()
{
  loadComputeDescriptorSetLayouts();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size()),
    .pSetLayouts = m_descriptorSetLayouts.data()
  };

  m_pipelineLayout = m_logicalDevice->createPipelineLayout(pipelineLayoutInfo);
}

void ComputePipeline::createPipeline()
{
  createPipelineLayout();

  loadComputeShaders();

  const VkComputePipelineCreateInfo computePipelineCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = m_shaderModule->getShaderStageCreateInfo(),
    .layout = m_pipelineLayout
  };

  m_pipeline = m_logicalDevice->createPipeline(computePipelineCreateInfo);

  m_shaderModule.reset();
}
