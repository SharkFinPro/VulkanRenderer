#ifndef VULKANPROJECT_PIPELINECONFIG_H
#define VULKANPROJECT_PIPELINECONFIG_H

#include "common/GraphicsPipelineStates.h"
#include "../GraphicsPipeline.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createUIPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                         const std::shared_ptr<RenderPass>& renderPass)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/ui.vert.spv",
        .fragmentShader = "assets/shaders/ui.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .renderPass = renderPass
    };
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG_H