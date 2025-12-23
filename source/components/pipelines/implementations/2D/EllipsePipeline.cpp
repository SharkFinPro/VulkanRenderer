#include "EllipsePipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../renderingManager/renderer2D/Renderer2D.h"

namespace vke {
  EllipsePipeline::EllipsePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                   std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/2D/Ellipse.vert.spv",
        .fragmentShader = "assets/shaders/2D/Ellipse.frag.spv"
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
            .size = sizeof(EllipsePushConstant)
          }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void EllipsePipeline::render(const RenderInfo* renderInfo,
                               const std::vector<Ellipse>* ellipses)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    for (const auto& ellipse : *ellipses)
    {
      renderEllipse(renderInfo, ellipse);
    }
  }

  void EllipsePipeline::renderEllipse(const RenderInfo* renderInfo,
                                      const Ellipse& ellipse) const
  {
    const EllipsePushConstant ellipsePC {
      .transform = ellipse.transform,
      .screenWidth = static_cast<int>(renderInfo->extent.width),
      .screenHeight = static_cast<int>(renderInfo->extent.height),
      .z = ellipse.z,
      .x = ellipse.bounds.x,
      .y = ellipse.bounds.y,
      .width = ellipse.bounds.z,
      .height = ellipse.bounds.w,
      .r = ellipse.color.r,
      .g = ellipse.color.g,
      .b = ellipse.color.b,
      .a = ellipse.color.a
    };

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             0, sizeof(EllipsePushConstant), &ellipsePC);

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }
} // vke