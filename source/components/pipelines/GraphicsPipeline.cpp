#include "GraphicsPipeline.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../assets/objects/RenderObject.h"
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

  void GraphicsPipeline::render(const RenderInfo* renderInfo,
                                const std::vector<std::shared_ptr<RenderObject>>* objects)
  {
    updateUniformVariables(renderInfo);

    renderInfo->commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    bindDescriptorSet(renderInfo);

    if (objects)
    {
      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

        object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame);
      }
    }
  }

  void GraphicsPipeline::bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  }

  void GraphicsPipeline::bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           const VkDescriptorSet descriptorSet,
                                           const uint32_t location) const
  {
    commandBuffer->bindDescriptorSets(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_pipelineLayout,
      location,
      1,
      &descriptorSet
    );
  }

  void GraphicsPipeline::createPipelineLayout(const GraphicsPipelineOptions& graphicsPipelineOptions)
  {
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
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

    const bool hasColorFormat = graphicsPipelineOptions.colorFormat != VK_FORMAT_UNDEFINED;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .viewMask = graphicsPipelineOptions.renderToCubeMap ? 0x3Fu : 0,
      .colorAttachmentCount = static_cast<uint32_t>(hasColorFormat),
      .pColorAttachmentFormats = hasColorFormat ? &graphicsPipelineOptions.colorFormat : nullptr,
      .depthAttachmentFormat = hasColorFormat ? m_logicalDevice->getPhysicalDevice()->findDepthFormat() : VK_FORMAT_D32_SFLOAT
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
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
      .layout = m_pipelineLayout,
      .renderPass = graphicsPipelineOptions.renderPass ? graphicsPipelineOptions.renderPass->getRenderPass() : VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1
    };

    m_pipeline = m_logicalDevice->createPipeline(pipelineInfo);
  }

} // namespace vke