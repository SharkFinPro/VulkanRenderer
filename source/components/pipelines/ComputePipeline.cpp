#include "ComputePipeline.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {

  void ComputePipeline::createPipelineLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                             const ComputePipelineOptions& computePipelineOptions)
  {
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = static_cast<uint32_t>(computePipelineOptions.descriptorSetLayouts.size()),
      .pSetLayouts = computePipelineOptions.descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(computePipelineOptions.pushConstantRanges.size()),
      .pPushConstantRanges = computePipelineOptions.pushConstantRanges.empty() ? nullptr : computePipelineOptions.pushConstantRanges.data()
    };

    m_pipelineLayout = logicalDevice->createPipelineLayout(pipelineLayoutInfo);
  }

  void ComputePipeline::createPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                       const ComputePipelineOptions& computePipelineOptions)
  {
    createPipelineLayout(logicalDevice, computePipelineOptions);

    const auto shaderModule = computePipelineOptions.shaders.getShaderModule(logicalDevice);

    const vk::ComputePipelineCreateInfo computePipelineCreateInfo {
      .stage = shaderModule.getShaderStageCreateInfo(),
      .layout = *m_pipelineLayout
    };

    m_pipeline = logicalDevice->createPipeline(computePipelineCreateInfo);
  }

} // namespace vke