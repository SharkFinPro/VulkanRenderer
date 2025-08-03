#include "ObjectHighlightPipeline.h"
#include "../config/GraphicsPipelineStates.h"
#include "../../RenderPass.h"
#include "../../../components/core/commandBuffer/CommandBuffer.h"
#include "../../../components/core/logicalDevice/LogicalDevice.h"
#include "../../../components/objects/RenderObject.h"

ObjectHighlightPipeline::ObjectHighlightPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                 const std::shared_ptr<RenderPass>& renderPass,
                                                 VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice), m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/ObjectHighlight.vert.spv",
      .fragmentShader = "assets/shaders/ObjectHighlight.frag.spv"
    },
    .states {
      .colorBlendState = GraphicsPipelineStates::colorBlendStateDots,
      .depthStencilState = GraphicsPipelineStates::depthStencilState,
      .dynamicState = GraphicsPipelineStates::dynamicState,
      .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
      .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
      .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
      .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
      .viewportState = GraphicsPipelineStates::viewportState
    },
    .renderPass = renderPass->getRenderPass()
  };

  createPipeline(graphicsPipelineOptions);
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

void ObjectHighlightPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
}
