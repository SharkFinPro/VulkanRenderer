#include "GridPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "../../components/core/commandBuffer/CommandBuffer.h"

namespace vke {
  GridPipeline::GridPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Grid.vert.spv",
        .fragmentShader = "assets/shaders/Grid.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void GridPipeline::render(const RenderInfo* renderInfo)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }
}
