#include "ObjectHighlightPipeline.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/RenderObject.h"

ObjectHighlightPipeline::ObjectHighlightPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                 const std::shared_ptr<RenderPass>& renderPass,
                                                 VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice), objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createPipeline(renderPass->getRenderPass());
}

void ObjectHighlightPipeline::render(const RenderInfo* renderInfo,
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

      object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
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
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}
