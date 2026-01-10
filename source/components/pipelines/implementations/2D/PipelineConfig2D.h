#ifndef VULKANPROJECT_PIPELINECONFIG2D_H
#define VULKANPROJECT_PIPELINECONFIG2D_H

#include "../common/GraphicsPipelineStates.h"
#include "../../GraphicsPipeline.h"
#include "../../../renderingManager/renderer2D/Primitives2D.h"

namespace vke::PipelineConfig {

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

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG2D_H