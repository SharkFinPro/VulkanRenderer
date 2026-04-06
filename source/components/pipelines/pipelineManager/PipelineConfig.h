#ifndef VULKANPROJECT_PIPELINECONFIG_H
#define VULKANPROJECT_PIPELINECONFIG_H

#include "../implementations/common/GraphicsPipelineStates.h"
#include "../GraphicsPipeline.h"
#include "../../renderingManager/renderer3D/Renderer3D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createGridPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           const std::shared_ptr<RenderPass>& renderPass)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/Grid.vert.spv",
        .fragmentShader = "assets/shaders/Grid.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateDots,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(GridPushConstant)
        }
      },
      .renderPass = renderPass
    };
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG_H