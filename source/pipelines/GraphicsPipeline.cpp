#include "GraphicsPipeline.h"
#include "ShaderModule.h"
#include "../components/LogicalDevice.h"
#include <stdexcept>

GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                   const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Pipeline(physicalDevice, logicalDevice)
{}

void GraphicsPipeline::createShader(const char* filename, VkShaderStageFlagBits stage)
{
  shaderModules.emplace_back(std::make_unique<ShaderModule>(logicalDevice, filename, stage));
}

void GraphicsPipeline::defineColorBlendState(VkPipelineColorBlendStateCreateInfo state)
{
  colorBlendState = std::make_unique<VkPipelineColorBlendStateCreateInfo>(state);
}

void GraphicsPipeline::defineDepthStencilState(VkPipelineDepthStencilStateCreateInfo state)
{
  depthStencilState = std::make_unique<VkPipelineDepthStencilStateCreateInfo>(state);
}

void GraphicsPipeline::defineDynamicState(VkPipelineDynamicStateCreateInfo state)
{
  dynamicState = std::make_unique<VkPipelineDynamicStateCreateInfo>(state);
}

void GraphicsPipeline::defineInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo state)
{
  inputAssemblyState = std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(state);
}

void GraphicsPipeline::defineMultisampleState(VkPipelineMultisampleStateCreateInfo state)
{
  multisampleState = std::make_unique<VkPipelineMultisampleStateCreateInfo>(state);
}

void GraphicsPipeline::defineRasterizationState(VkPipelineRasterizationStateCreateInfo state)
{
  rasterizationState = std::make_unique<VkPipelineRasterizationStateCreateInfo>(state);
}

void GraphicsPipeline::defineTessellationState(VkPipelineTessellationStateCreateInfo state)
{
  tessellationState = std::make_unique<VkPipelineTessellationStateCreateInfo>(state);
}

void GraphicsPipeline::defineVertexInputState(VkPipelineVertexInputStateCreateInfo state)
{
  vertexInputState = std::make_unique<VkPipelineVertexInputStateCreateInfo>(state);
}

void GraphicsPipeline::defineViewportState(VkPipelineViewportStateCreateInfo state)
{
  viewportState = std::make_unique<VkPipelineViewportStateCreateInfo>(state);
}

void GraphicsPipeline::loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
  descriptorSetLayouts.emplace_back(descriptorSetLayout);
}

void GraphicsPipeline::createPipelineLayout()
{
  loadGraphicsDescriptorSetLayouts();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.data()
  };

  if (vkCreatePipelineLayout(logicalDevice->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  descriptorSetLayouts.clear();
}

void GraphicsPipeline::createPipeline(const VkRenderPass& renderPass)
{
  createPipelineLayout();

  defineStates();

  loadGraphicsShaders();

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  for (const auto& shader : shaderModules)
  {
    shaderStages.push_back(shader->getShaderStageCreateInfo());
  }

  const VkGraphicsPipelineCreateInfo pipelineInfo {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = static_cast<uint32_t>(shaderStages.size()),
    .pStages = shaderStages.data(),
    .pVertexInputState = vertexInputState.get(),
    .pInputAssemblyState = inputAssemblyState.get(),
    .pTessellationState = tessellationState.get(),
    .pViewportState = viewportState.get(),
    .pRasterizationState = rasterizationState.get(),
    .pMultisampleState = multisampleState.get(),
    .pDepthStencilState = depthStencilState.get(),
    .pColorBlendState = colorBlendState.get(),
    .pDynamicState = dynamicState.get(),
    .layout = pipelineLayout,
    .renderPass = renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  if (vkCreateGraphicsPipelines(logicalDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  shaderModules.clear();
}
