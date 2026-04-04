#include "GraphicsPipeline.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../renderingManager/legacyRenderer/RenderPass.h"

namespace vke {

  GraphicsPipeline::GraphicsPipeline(std::shared_ptr<LogicalDevice> logicalDevice)
    : Pipeline(std::move(logicalDevice))
  {}

  GraphicsPipeline::GraphicsPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                     const GraphicsPipelineOptions& graphicsPipelineOptions)
    : Pipeline(std::move(logicalDevice))
  {
    createPipeline(graphicsPipelineOptions);
  }

  void GraphicsPipeline::bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
  }

  void GraphicsPipeline::bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           const vk::DescriptorSet descriptorSet,
                                           const uint32_t location) const
  {
    commandBuffer->bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      m_pipelineLayout,
      location,
      { descriptorSet }
    );
  }

  void GraphicsPipeline::createPipelineLayout(const GraphicsPipelineOptions& graphicsPipelineOptions)
  {
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = static_cast<uint32_t>(graphicsPipelineOptions.descriptorSetLayouts.size()),
      .pSetLayouts = graphicsPipelineOptions.descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(graphicsPipelineOptions.pushConstantRanges.size()),
      .pPushConstantRanges = graphicsPipelineOptions.pushConstantRanges.empty() ? nullptr : graphicsPipelineOptions.pushConstantRanges.data()
    };

    m_pipelineLayout = m_logicalDevice->createPipelineLayout(pipelineLayoutInfo);
  }

  void GraphicsPipeline::createPipeline(const GraphicsPipelineOptions& graphicsPipelineOptions)
  {
    createPipelineLayout(graphicsPipelineOptions);

    const auto shaderModules = graphicsPipelineOptions.shaders.getShaderModules(m_logicalDevice);
    const auto shaderStages = graphicsPipelineOptions.shaders.getShaderStages(shaderModules);

    const bool hasColorFormat = graphicsPipelineOptions.colorFormat != vk::Format::eUndefined;

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo {
      .viewMask = graphicsPipelineOptions.renderToCubeMap ? 0x3Fu : 0u,
      .colorAttachmentCount = static_cast<uint32_t>(hasColorFormat),
      .pColorAttachmentFormats = hasColorFormat ? &graphicsPipelineOptions.colorFormat : nullptr,
      .depthAttachmentFormat = hasColorFormat ? m_logicalDevice->getPhysicalDevice()->findDepthFormat() : vk::Format::eD32Sfloat
    };

    const vk::GraphicsPipelineCreateInfo pipelineInfo {
      .pNext = graphicsPipelineOptions.renderPass ? nullptr : &pipelineRenderingCreateInfo,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &graphicsPipelineOptions.states.vertexInputState,
      .pInputAssemblyState = &graphicsPipelineOptions.states.inputAssemblyState,
      .pTessellationState = &graphicsPipelineOptions.states.tessellationState,
      .pViewportState = &graphicsPipelineOptions.states.viewportState,
      .pRasterizationState = &graphicsPipelineOptions.states.rasterizationState,
      .pMultisampleState = &graphicsPipelineOptions.states.multisampleState,
      .pDepthStencilState = &graphicsPipelineOptions.states.depthStencilState,
      .pColorBlendState = &graphicsPipelineOptions.states.colorBlendState,
      .pDynamicState = &graphicsPipelineOptions.states.dynamicState,
      .layout = *m_pipelineLayout,
      .renderPass = graphicsPipelineOptions.renderPass ? graphicsPipelineOptions.renderPass->getRenderPass() : nullptr,
      .subpass = 0,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1
    };

    m_pipeline = m_logicalDevice->createPipeline(pipelineInfo);
  }

} // namespace vke