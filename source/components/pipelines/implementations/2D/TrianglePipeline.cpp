#include "TrianglePipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../renderingManager/renderer2D/Renderer2D.h"

namespace vke {
  TrianglePipeline::TrianglePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                     std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/2D/Triangle.vert.spv",
        .fragmentShader = "assets/shaders/2D/Triangle.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilStateNone,
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
            .size = sizeof(TrianglePushConstant)
          }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void TrianglePipeline::render(const RenderInfo* renderInfo,
                                const std::vector<Triangle>* triangles)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    for (const auto& triangle : *triangles)
    {
      renderTriangle(renderInfo, triangle);
    }
  }

  void TrianglePipeline::renderTriangle(const RenderInfo* renderInfo,
                                        const Triangle& triangle) const
  {
    const TrianglePushConstant trianglePC {
      .r = triangle.color.r,
      .g = triangle.color.g,
      .b = triangle.color.b,
      .a = triangle.color.a,
      .transform = triangle.transform,
      .screenWidth = static_cast<int>(renderInfo->extent.width),
      .screenHeight = static_cast<int>(renderInfo->extent.height),
      .z = triangle.z,
      .x1 = triangle.p1.x,
      .y1 = triangle.p1.y,
      .x2 = triangle.p2.x,
      .y2 = triangle.p2.y,
      .x3 = triangle.p3.x,
      .y3 = triangle.p3.y
    };

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             0, sizeof(TrianglePushConstant), &trianglePC);

    renderInfo->commandBuffer->draw(3, 1, 0, 0);
  }
} // vke