#include "FontPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../assets/AssetManager.h"
#include "../../../assets/fonts/Font.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../renderingManager/renderer2D/Renderer2D.h"

namespace vke {
  FontPipeline::FontPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass,
                             VkDescriptorSetLayout fontDescriptorSetLayout)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/2D/Font.vert.spv",
        .fragmentShader = "assets/shaders/2D/Font.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
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
      .descriptorSetLayouts {
        fontDescriptorSetLayout
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void FontPipeline::render(const RenderInfo* renderInfo,
                            const std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Glyph>>>* glyphs,
                            const std::shared_ptr<AssetManager>& assetManager)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    for (const auto& [fontName, fontSizes] : *glyphs)
    {
      for (const auto& [fontSize, text] : fontSizes)
      {
        auto descriptorSet = assetManager->getFont(fontName, fontSize)->getDescriptorSet(renderInfo->currentFrame);

        renderInfo->commandBuffer->bindDescriptorSets(
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          m_pipelineLayout,
          0,
          1,
          &descriptorSet
        );

        for (const auto& glyph : text)
        {
          renderGlyph(renderInfo, glyph);
        }
      }
    }
  }

  void FontPipeline::renderGlyph(const RenderInfo* renderInfo,
                                 const Glyph& glyph) const
  {
    const GlyphPushConstant glyphPC {
      .transform = glyph.transform,
      .screenWidth = static_cast<int>(renderInfo->extent.width),
      .screenHeight = static_cast<int>(renderInfo->extent.height),
      .z = glyph.z,
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