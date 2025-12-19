#include "RectPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../renderingManager/renderer2D/Renderer2D.h"

namespace vke {
  RectPipeline::RectPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/2D/rect.vert.spv",
        .fragmentShader = "assets/shaders/2D/rect.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilStateNone,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(RectPushConstant)
        }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void RectPipeline::render(const RenderInfo* renderInfo,
                            const std::vector<Rect>* rects)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    for (const auto& rect : *rects)
    {
      renderRect(renderInfo, rect);
    }
  }

  void RectPipeline::renderRect(const RenderInfo* renderInfo, const Rect& rect) const
  {
    const RectPushConstant rectPC {
      .transform = rect.transform,
      .screenSize = {
        renderInfo->extent.width,
        renderInfo->extent.height
      },
      .bounds = rect.bounds,
      .color = rect.color
    };

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             0, sizeof(RectPushConstant), &rectPC);

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }
} // vke