#ifndef VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H
#define VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H

#include "../implementations/common/GraphicsPipelineStates.h"
#include "../GraphicsPipeline.h"
#include "../../lighting/lights/Light.h"
#include "../../renderingManager/renderer3D/Renderer3D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createTexturedPlanePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                    vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                    vk::DescriptorSetLayout materialDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/TexturedPlane.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/TexturedPlane.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createMagnifyWhirlMosaicPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                         vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                         vk::DescriptorSetLayout materialDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/MagnifyWhirlMosaic.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(MagnifyWhirlMosaicPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createMousePickingPipelineOptions(vk::DescriptorSetLayout sceneDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/MousePicking.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/MousePicking.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::multisampleStateNone,
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertexPositionOnly,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout
      },
      .colorFormat = vk::Format::eR32G32B32A32Uint
    };
  }

  inline GraphicsPipelineOptions createMousePickingAlphaTestedPipelineOptions(vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                              vk::DescriptorSetLayout materialDescriptorSetLayout)
  {
    auto options = createMousePickingPipelineOptions(sceneDescriptorSetLayout);

    options.descriptorSetLayouts.push_back(materialDescriptorSetLayout);

    options.shaders.vertexShader = "assets/shaders/renderObject/MousePickingAlphaTested.vert.spv";
    options.shaders.fragmentShader = "assets/shaders/renderObject/MousePickingAlphaTested.frag.spv";
    options.states.rasterizationState = gps::rasterizationStateNoCull;
    options.states.vertexInputState = gps::vertexInputStateVertex;

    return options;
  }

  inline GraphicsPipelineOptions createOutlineMaskPipelineOptions(vk::DescriptorSetLayout sceneDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/MousePicking.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/OutlineMask.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilStateNone,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::multisampleStateNone,
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateVertexPositionOnly,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(uint32_t)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout
      },
      .colorFormat = vk::Format::eR8Uint,
      .hasDepthAttachment = false
    };
  }

  inline GraphicsPipelineOptions createShadowMapPipelineOptions(vk::DescriptorSetLayout sceneDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Shadow.vert.spv",
      },
      .states {
        .colorBlendState = gps::colorBlendStateShadow,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::multisampleStateNone,
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex,
          .offset = 0,
          .size = sizeof(glm::mat4)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout
      },
      .colorFormat = vk::Format::eUndefined,
      .depthFormat = Light::s_shadowMapFormat
    };
  }

  inline GraphicsPipelineOptions createPointLightShadowMapPipelineOptions(vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                          vk::DescriptorSetLayout pointLightDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/ShadowCubeMap.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/ShadowCubeMap.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateShadow,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::multisampleStateNone,
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(glm::vec3)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        pointLightDescriptorSetLayout
      },
      .colorFormat = vk::Format::eUndefined,
      .depthFormat = Light::s_shadowMapFormat,
      .renderToCubeMap = true
    };
  }

  inline GraphicsPipelineOptions createEllipticalDotsPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                     vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                     vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                                     vk::DescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/EllipticalDots.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(EllipticalDotsPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createCrossesPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                              vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                              vk::DescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Crosses.vert.spv",
        .geometryShader = "assets/shaders/renderObject/Crosses.geom.spv",
        .fragmentShader = "assets/shaders/renderObject/Crosses.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertexPositionAndNormal,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(CrossesPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createCurtainPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                              vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                              vk::DescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Curtain.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/Curtain.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(CurtainPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createObjectsPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                              vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                              vk::DescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/objects.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createSnakePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                            vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                            vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                            vk::DescriptorSetLayout lightingDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Snake.vert.spv",
        .geometryShader = "assets/shaders/renderObject/Snake.geom.spv",
        .fragmentShader = "assets/shaders/renderObject/Snake.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(SnakePushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createNoisyEllipticalDotsPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                          vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                          vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                                          vk::DescriptorSetLayout lightingDescriptorSetLayout,
                                                                          vk::DescriptorSetLayout noiseDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/NoisyEllipticalDots.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(NoisyEllipticalDotsPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout,
        noiseDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createBumpyCurtainPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                   vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                                   vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                                   vk::DescriptorSetLayout lightingDescriptorSetLayout,
                                                                   vk::DescriptorSetLayout noiseDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Curtain.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/BumpyCurtain.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(BumpyCurtainPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightingDescriptorSetLayout,
        noiseDescriptorSetLayout
      }
    };
  }

  inline GraphicsPipelineOptions createCubeMapPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              vk::DescriptorSetLayout sceneDescriptorSetLayout,
                                                              vk::DescriptorSetLayout materialDescriptorSetLayout,
                                                              vk::DescriptorSetLayout cubeMapDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/CubeMap.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(CubeMapPushConstant)
        }
      },
      .descriptorSetLayouts {
        sceneDescriptorSetLayout,
        materialDescriptorSetLayout,
        cubeMapDescriptorSetLayout
      }
    };
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIGRENDEROBJECT_H