#include "ObjectHighlightPipeline.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include "../../objects/RenderObject.h"

ObjectHighlightPipeline::ObjectHighlightPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                 const std::shared_ptr<RenderPass>& renderPass,
                                                 VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(physicalDevice, logicalDevice), objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createPipeline(renderPass->getRenderPass());
}

void ObjectHighlightPipeline::render(const RenderInfo* renderInfo,
                                     const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  updateUniformVariables(renderInfo);

  vkCmdBindPipeline(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  bindDescriptorSet(renderInfo);

  if (objects)
  {
    for (const auto& object : *objects)
    {
      object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      object->draw(renderInfo->commandBuffer, pipelineLayout, renderInfo->currentFrame, true);
    }
  }
}

void ObjectHighlightPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/ObjectHighlight.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/ObjectHighlight.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ObjectHighlightPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void ObjectHighlightPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateDots);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}
