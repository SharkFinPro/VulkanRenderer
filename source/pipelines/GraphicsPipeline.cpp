#include "GraphicsPipeline.h"

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                                   std::shared_ptr<LogicalDevice> logicalDevice)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{}

GraphicsPipeline::~GraphicsPipeline()
{
  vkDestroyPipeline(logicalDevice->getDevice(), pipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice->getDevice(), pipelineLayout, nullptr);
}

void GraphicsPipeline::createShader(const char *filename, VkShaderStageFlagBits stage)
{
  shaderModules.emplace_back(std::make_unique<ShaderModule>(logicalDevice->getDevice(), filename, stage));
}

void GraphicsPipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void GraphicsPipeline::createPipelineLayout()
{
  loadDescriptorSetLayouts();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  descriptorSetLayouts.clear();
}

void GraphicsPipeline::createPipeline(const VkRenderPass& renderPass)
{
  createPipelineLayout();

  const auto colorBlendState = defineColorBlendState();
  const auto depthStencilState = defineDepthStencilState();
  const auto dynamicState = defineDynamicState();
  const auto inputAssemblyState = defineInputAssemblyState();
  const auto multisampleState = defineMultisampleState();
  const auto rasterizationState = defineRasterizationState();
  const auto tessellationState = defineTessellationState();
  const auto vertexInputState = defineVertexInputState();
  const auto viewportState = defineViewportState();

  loadShaders();

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  for (const auto& shader : shaderModules)
  {
    shaderStages.push_back(shader->getShaderStageCreateInfo());
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pColorBlendState = colorBlendState.get();
  pipelineInfo.pDepthStencilState = depthStencilState.get();
  pipelineInfo.pDynamicState = dynamicState.get();
  pipelineInfo.pInputAssemblyState = inputAssemblyState.get();
  pipelineInfo.pMultisampleState = multisampleState.get();
  pipelineInfo.pRasterizationState = rasterizationState.get();
  pipelineInfo.pTessellationState = tessellationState.get();
  pipelineInfo.pVertexInputState = vertexInputState.get();
  pipelineInfo.pViewportState = viewportState.get();
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  shaderModules.clear();
}
