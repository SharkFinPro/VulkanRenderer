#include "ObjectHighlightPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include "../../../assets/objects/RenderObject.h"

namespace vke {

ObjectHighlightPipeline::ObjectHighlightPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                 std::shared_ptr<RenderPass> renderPass,
                                                 VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice)
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
    .descriptorSetLayouts {
      objectDescriptorSetLayout
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
}

void ObjectHighlightPipeline::render(const RenderInfo* renderInfo,
                                     const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, nullptr);

  for (const auto& object : *objects)
  {
    object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

    object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
  }
}

} // namespace vke