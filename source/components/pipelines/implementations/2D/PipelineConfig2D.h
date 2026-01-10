#ifndef VULKANPROJECT_PIPELINECONFIG2D_H
#define VULKANPROJECT_PIPELINECONFIG2D_H

#include "../common/GraphicsPipelineStates.h"
#include "../../GraphicsPipeline.h"
#include "../../../renderingManager/renderer2D/Primitives2D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createRectPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           const std::shared_ptr<RenderPass>& renderPass)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/rect.vert.spv",
        .fragmentShader = "assets/shaders/2D/rect.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(Rect::PushConstant)
        }
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createTrianglePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              const std::shared_ptr<RenderPass>& renderPass)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Triangle.vert.spv",
        .fragmentShader = "assets/shaders/2D/Triangle.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(logicalDevice),
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
  }

  inline GraphicsPipelineOptions createEllipsePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              const std::shared_ptr<RenderPass>& renderPass)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Ellipse.vert.spv",
        .fragmentShader = "assets/shaders/2D/Ellipse.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(Ellipse::PushConstant)
        }
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createFontPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           const std::shared_ptr<RenderPass>& renderPass,
                                                           VkDescriptorSetLayout fontDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Font.vert.spv",
        .fragmentShader = "assets/shaders/2D/Font.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateTransparent,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(logicalDevice),
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
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG2D_H