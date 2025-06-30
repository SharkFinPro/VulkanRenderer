#include "GraphicsPipeline.h"
#include "ShaderModule.h"
#include "../components/LogicalDevice.h"
#include "../objects/RenderObject.h"
#include <stdexcept>

GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                   const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Pipeline(physicalDevice, logicalDevice)
{}

void GraphicsPipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  updateUniformVariables(renderInfo);

  vkCmdBindPipeline(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  bindDescriptorSet(renderInfo);

  if (objects)
  {
    for (const auto& object : *objects)
    {
      object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      object->draw(renderInfo->commandBuffer, pipelineLayout, renderInfo->currentFrame);
    }
  }
}

void GraphicsPipeline::createShader(const char* filename, VkShaderStageFlagBits stage)
{
  shaderModules.emplace_back(std::make_unique<ShaderModule>(logicalDevice, filename, stage));
}

void GraphicsPipeline::defineColorBlendState(const VkPipelineColorBlendStateCreateInfo& state)
{
  colorBlendState = std::make_unique<VkPipelineColorBlendStateCreateInfo>(state);
}

void GraphicsPipeline::defineDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state)
{
  depthStencilState = std::make_unique<VkPipelineDepthStencilStateCreateInfo>(state);
}

void GraphicsPipeline::defineDynamicState(const VkPipelineDynamicStateCreateInfo& state)
{
  dynamicState = std::make_unique<VkPipelineDynamicStateCreateInfo>(state);
}

void GraphicsPipeline::defineInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state)
{
  inputAssemblyState = std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(state);
}

void GraphicsPipeline::defineMultisampleState(const VkPipelineMultisampleStateCreateInfo& state)
{
  multisampleState = std::make_unique<VkPipelineMultisampleStateCreateInfo>(state);
}

void GraphicsPipeline::defineRasterizationState(const VkPipelineRasterizationStateCreateInfo& state)
{
  rasterizationState = std::make_unique<VkPipelineRasterizationStateCreateInfo>(state);
}

void GraphicsPipeline::defineTessellationState(const VkPipelineTessellationStateCreateInfo& state)
{
  tessellationState = std::make_unique<VkPipelineTessellationStateCreateInfo>(state);
}

void GraphicsPipeline::defineVertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
  vertexInputState = std::make_unique<VkPipelineVertexInputStateCreateInfo>(state);
}

void GraphicsPipeline::defineViewportState(const VkPipelineViewportStateCreateInfo& state)
{
  viewportState = std::make_unique<VkPipelineViewportStateCreateInfo>(state);
}

void GraphicsPipeline::definePushConstantRange(VkPushConstantRange range)
{
  pushConstantRanges.emplace_back(range);
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
    .pSetLayouts = descriptorSetLayouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
    .pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data()
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

  destroyStates();
}

void GraphicsPipeline::destroyStates()
{
  colorBlendState.reset();
  depthStencilState.reset();
  dynamicState.reset();
  inputAssemblyState.reset();
  multisampleState.reset();
  rasterizationState.reset();
  tessellationState.reset();
  vertexInputState.reset();
  viewportState.reset();
}
