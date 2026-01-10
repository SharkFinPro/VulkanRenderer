#include "TrianglePipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../renderingManager/renderer2D/Primitives2D.h"

namespace vke {
  TrianglePipeline::TrianglePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                     std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(std::move(logicalDevice))
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/2D/Triangle.vert.spv",
        .fragmentShader = "assets/shaders/2D/Triangle.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
          {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(Triangle::PushConstant)
          }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }
} // vke