#include "GraphicsPipeline.h"
#include "ShaderModule.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../objects/RenderObject.h"

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

void GraphicsPipeline::createShader(const char* filename, VkShaderStageFlagBits stage)
{
  m_shaderModules.emplace_back(std::make_unique<ShaderModule>(m_logicalDevice, filename, stage));
}

void GraphicsPipeline::defineColorBlendState(const VkPipelineColorBlendStateCreateInfo& state)
{
  m_colorBlendState = std::make_unique<VkPipelineColorBlendStateCreateInfo>(state);
}

void GraphicsPipeline::defineDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state)
{
  m_depthStencilState = std::make_unique<VkPipelineDepthStencilStateCreateInfo>(state);
}

void GraphicsPipeline::defineDynamicState(const VkPipelineDynamicStateCreateInfo& state)
{
  m_dynamicState = std::make_unique<VkPipelineDynamicStateCreateInfo>(state);
}

void GraphicsPipeline::defineInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state)
{
  m_inputAssemblyState = std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(state);
}

void GraphicsPipeline::defineMultisampleState(const VkPipelineMultisampleStateCreateInfo& state)
{
  m_multisampleState = std::make_unique<VkPipelineMultisampleStateCreateInfo>(state);
}

void GraphicsPipeline::defineRasterizationState(const VkPipelineRasterizationStateCreateInfo& state)
{
  m_rasterizationState = std::make_unique<VkPipelineRasterizationStateCreateInfo>(state);
}

void GraphicsPipeline::defineTessellationState(const VkPipelineTessellationStateCreateInfo& state)
{
  m_tessellationState = std::make_unique<VkPipelineTessellationStateCreateInfo>(state);
}

void GraphicsPipeline::defineVertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
  m_vertexInputState = std::make_unique<VkPipelineVertexInputStateCreateInfo>(state);
}

void GraphicsPipeline::defineViewportState(const VkPipelineViewportStateCreateInfo& state)
{
  m_viewportState = std::make_unique<VkPipelineViewportStateCreateInfo>(state);
}

void GraphicsPipeline::definePushConstantRange(VkPushConstantRange range)
{
  m_pushConstantRanges.emplace_back(range);
}

void GraphicsPipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  m_descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void GraphicsPipeline::createPipelineLayout()
{
  loadGraphicsDescriptorSetLayouts();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size()),
    .pSetLayouts = m_descriptorSetLayouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size()),
    .pPushConstantRanges = m_pushConstantRanges.empty() ? nullptr : m_pushConstantRanges.data()
  };

  m_pipelineLayout = m_logicalDevice->createPipelineLayout(pipelineLayoutInfo);

  m_descriptorSetLayouts.clear();
}

void GraphicsPipeline::createPipeline(const VkRenderPass& renderPass)
{
  createPipelineLayout();

  defineStates();

  loadGraphicsShaders();

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  for (const auto& shader : m_shaderModules)
  {
    shaderStages.push_back(shader->getShaderStageCreateInfo());
  }

  const VkGraphicsPipelineCreateInfo pipelineInfo {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = static_cast<uint32_t>(shaderStages.size()),
    .pStages = shaderStages.data(),
    .pVertexInputState = m_vertexInputState.get(),
    .pInputAssemblyState = m_inputAssemblyState.get(),
    .pTessellationState = m_tessellationState.get(),
    .pViewportState = m_viewportState.get(),
    .pRasterizationState = m_rasterizationState.get(),
    .pMultisampleState = m_multisampleState.get(),
    .pDepthStencilState = m_depthStencilState.get(),
    .pColorBlendState = m_colorBlendState.get(),
    .pDynamicState = m_dynamicState.get(),
    .layout = m_pipelineLayout,
    .renderPass = renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  m_pipeline = m_logicalDevice->createPipeline(pipelineInfo);

  m_shaderModules.clear();

  destroyStates();
}

void GraphicsPipeline::destroyStates()
{
  m_colorBlendState.reset();
  m_depthStencilState.reset();
  m_dynamicState.reset();
  m_inputAssemblyState.reset();
  m_multisampleState.reset();
  m_rasterizationState.reset();
  m_tessellationState.reset();
  m_vertexInputState.reset();
  m_viewportState.reset();
}
