#include "FontPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../renderingManager/renderer2D/Primitives2D.h"

namespace vke {
  FontPipeline::FontPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                             std::shared_ptr<RenderPass> renderPass,
                             VkDescriptorSetLayout fontDescriptorSetLayout)
    : GraphicsPipeline(std::move(logicalDevice))
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
          .size = sizeof(Glyph::PushConstant)
        }
      },
      .descriptorSetLayouts {
        fontDescriptorSetLayout
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }
} // vke