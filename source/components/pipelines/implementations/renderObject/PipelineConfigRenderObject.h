#ifndef VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H
#define VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H

#include "../common/GraphicsPipelineStates.h"
#include "../../GraphicsPipeline.h"
#include "../../../renderingManager/renderer3D/Renderer3D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createTexturedPlanePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                    const std::shared_ptr<RenderPass>& renderPass,
                                                                    const VkDescriptorSetLayout objectDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/TexturedPlane.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/TexturedPlane.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createObjectHighlightPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                    const std::shared_ptr<RenderPass>& renderPass,
                                                                    const VkDescriptorSetLayout objectDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/ObjectHighlight.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/ObjectHighlight.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateDots,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertexPositionOnly,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createMagnifyWhirlMosaicPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                         const std::shared_ptr<RenderPass>& renderPass,
                                                                         const VkDescriptorSetLayout objectDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/MagnifyWhirlMosaic.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(MagnifyWhirlMosaicPushConstant)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

}

#endif //VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H