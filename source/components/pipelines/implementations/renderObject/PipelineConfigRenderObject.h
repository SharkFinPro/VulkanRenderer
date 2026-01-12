#ifndef VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H
#define VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H

#include "../common/GraphicsPipelineStates.h"
#include "../../GraphicsPipeline.h"
#include "../../../renderingManager/renderer3D/Renderer3D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createTexturedPlanePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                    const std::shared_ptr<RenderPass>& renderPass,
                                                                    VkDescriptorSetLayout objectDescriptorSetLayout)
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
                                                                    VkDescriptorSetLayout objectDescriptorSetLayout)
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
                                                                         VkDescriptorSetLayout objectDescriptorSetLayout)
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

  inline GraphicsPipelineOptions createMousePickingPipelineOptions(const std::shared_ptr<RenderPass>& renderPass,
                                                                   VkDescriptorSetLayout objectDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/MousePicking.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/MousePicking.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::multisampleStateNone,
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertexPositionOnly,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(uint32_t)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass,
      .colorFormat = VK_FORMAT_R8G8B8A8_UINT
    };
  }

  inline GraphicsPipelineOptions createShadowMapPipelineOptions(const std::shared_ptr<RenderPass>& renderPass,
                                                                VkDescriptorSetLayout objectDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Shadow.vert.spv",
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateShadow,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::multisampleStateNone,
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = sizeof(glm::mat4)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass,
      .colorFormat = VK_FORMAT_UNDEFINED
    };
  }

  inline GraphicsPipelineOptions createPointLightShadowMapPipelineOptions(const std::shared_ptr<RenderPass>& renderPass,
                                                                          VkDescriptorSetLayout objectDescriptorSetLayout,
                                                                          VkDescriptorSetLayout pointLightDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/ShadowCubeMap.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/ShadowCubeMap.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateShadow,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::multisampleStateNone,
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(glm::vec3)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout,
        pointLightDescriptorSetLayout
      },
      .renderPass = renderPass,
      .colorFormat = VK_FORMAT_UNDEFINED,
      .renderToCubeMap = true
    };
  }

  inline GraphicsPipelineOptions createEllipticalDotsPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                     const std::shared_ptr<RenderPass>& renderPass,
                                                                     VkDescriptorSetLayout objectDescriptorSetLayout,
                                                                     VkDescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/EllipticalDots.frag.spv"
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
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(EllipticalDotsPushConstant)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout,
        lightingDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createCrossesPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                     const std::shared_ptr<RenderPass>& renderPass,
                                                                     VkDescriptorSetLayout objectDescriptorSetLayout,
                                                                     VkDescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Crosses.vert.spv",
        .geometryShader = "assets/shaders/renderObject/Crosses.geom.spv",
        .fragmentShader = "assets/shaders/renderObject/Crosses.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertexPositionAndNormal,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(CrossesPushConstant)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout,
        lightingDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

  inline GraphicsPipelineOptions createCurtainPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              const std::shared_ptr<RenderPass>& renderPass,
                                                              VkDescriptorSetLayout objectDescriptorSetLayout,
                                                              VkDescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Curtain.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/Curtain.frag.spv"
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
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(CurtainPushConstant)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout,
        lightingDescriptorSetLayout
      },
      .renderPass = renderPass
    };
  }

}

#endif //VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H