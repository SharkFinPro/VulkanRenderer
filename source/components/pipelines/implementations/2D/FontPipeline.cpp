#include "FontPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../renderingManager/renderer2D/Renderer2D.h"

namespace vke {
  FontPipeline::FontPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "shaders/2D/Font.vert.spv",
        .fragmentShader = "shaders/2D/Font.frag.spv"
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
          .size = sizeof(GlyphPushConstant)
        }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void FontPipeline::render(const RenderInfo* renderInfo,
                            const std::vector<Glyph>* glyphs)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    for (const auto& glyph : *glyphs)
    {
      renderGlyph(renderInfo, glyph);
    }
  }

  void FontPipeline::renderGlyph(const RenderInfo* renderInfo,
                                 const Glyph& glyph) const
  {
    const GlyphPushConstant glyphPC {
      .transform = glyph.transform,
      .screenWidth = static_cast<int>(renderInfo->extent.width),
      .screenHeight = static_cast<int>(renderInfo->extent.height),
      .x = glyph.bounds.x,
      .y = glyph.bounds.y,
      .width = glyph.bounds.z,
      .height = glyph.bounds.w,
      .u0 = glyph.uv.x,
      .v0 = glyph.uv.y,
      .u1 = glyph.uv.z,
      .v1 = glyph.uv.w,
      .r = glyph.color.r,
      .g = glyph.color.g,
      .b = glyph.color.b,
      .a = glyph.color.a
    };

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             0, sizeof(GlyphPushConstant), &glyphPC);

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }
} // vke