#include "ComputePipeline.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {

  ComputePipeline::ComputePipeline(std::shared_ptr<LogicalDevice> logicalDevice)
    : Pipeline(std::move(logicalDevice))
  {}

  void ComputePipeline::createPipelineLayout(const ComputePipelineOptions& computePipelineOptions)
  {
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = static_cast<uint32_t>(computePipelineOptions.descriptorSetLayouts.size()),
      .pSetLayouts = computePipelineOptions.descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(computePipelineOptions.pushConstantRanges.size()),
      .pPushConstantRanges = computePipelineOptions.pushConstantRanges.empty() ? nullptr : computePipelineOptions.pushConstantRanges.data()
    };

    m_pipelineLayout = m_logicalDevice->createPipelineLayout(pipelineLayoutInfo);
  }

  void ComputePipeline::createPipeline(const ComputePipelineOptions& computePipelineOptions)
  {
    createPipelineLayout(computePipelineOptions);

    const auto shaderModule = computePipelineOptions.shaders.getShaderModule(m_logicalDevice);

    const vk::ComputePipelineCreateInfo computePipelineCreateInfo {
      .stage = shaderModule.getShaderStageCreateInfo(),
      .layout = *m_pipelineLayout
    };

    m_pipeline = m_logicalDevice->createPipeline(computePipelineCreateInfo);
  }

} // namespace vke