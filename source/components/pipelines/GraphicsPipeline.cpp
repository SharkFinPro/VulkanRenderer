#include "GraphicsPipeline.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../assets/objects/RenderObject.h"
#include "../renderingManager/legacyRenderer/renderPass/RenderPass.h"

namespace vke {

GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Pipeline(logicalDevice)
{}

void GraphicsPipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
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

void GraphicsPipeline::createPipeline(const GraphicsPipelineOptions& graphicsPipelineOptions, const bool useColorAttachment)
{
  createPipelineLayout(graphicsPipelineOptions);

  const auto shaderModules = graphicsPipelineOptions.shaders.getShaderModules(m_logicalDevice);
  const auto shaderStages = graphicsPipelineOptions.shaders.getShaderStages(shaderModules);

  constexpr VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = static_cast<uint32_t>(useColorAttachment ? 1 : 0),
    .pColorAttachmentFormats = useColorAttachment ? &colorFormat : nullptr,
    .depthAttachmentFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat()
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