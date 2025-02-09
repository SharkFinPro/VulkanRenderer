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

  const auto colorBlendState = defineColorBlendState();
  const auto depthStencilState = defineDepthStencilState();
  const auto dynamicState = defineDynamicState();
  const auto inputAssemblyState = defineInputAssemblyState();
  const auto multisampleState = defineMultisampleState();
  const auto rasterizationState = defineRasterizationState();
  const auto tessellationState = defineTessellationState();
  const auto vertexInputState = defineVertexInputState();
  const auto viewportState = defineViewportState();

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
