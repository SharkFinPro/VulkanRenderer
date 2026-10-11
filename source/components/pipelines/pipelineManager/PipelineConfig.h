#ifndef VULKANPROJECT_PIPELINECONFIG_H
#define VULKANPROJECT_PIPELINECONFIG_H

#include "../implementations/common/GraphicsPipelineStates.h"
#include "../GraphicsPipeline.h"
#include "../../renderingManager/renderer3D/Renderer3D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createGridPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/Grid.vert.spv",
        .fragmentShader = "assets/shaders/Grid.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateDots,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(GridPushConstant)
        }
      }
    };
  }

  inline GraphicsPipelineOptions createOffscreenToSwapchainPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                           const vk::DescriptorSetLayout offscreenImageDescriptorSetLayout,
                                                                           const vk::Format swapchainImageFormat)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/OffscreenToSwapchain.vert.spv",
        .fragmentShader = "assets/shaders/OffscreenToSwapchain.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilStateNone,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        offscreenImageDescriptorSetLayout
      },
      .colorFormat = swapchainImageFormat
    };
  }

  // Draws the ray traced color and depth into the scene pass.
  inline GraphicsPipelineOptions createRayTracingCompositePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                          const vk::DescriptorSetLayout rayTracingImagesDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/Outline.vert.spv",
        .fragmentShader = "assets/shaders/RayTracingComposite.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilStateWriteAlways,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        rayTracingImagesDescriptorSetLayout
      }
    };
  }

  // The outline composite is drawn into the multisampled scene pass.
  inline GraphicsPipelineOptions createOutlinePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                              const vk::DescriptorSetLayout maskDescriptorSetLayout,
                                                              const vk::DescriptorSetLayout colorsDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/Outline.vert.spv",
        .fragmentShader = "assets/shaders/Outline.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateOverlay,
        .depthStencilState = gps::depthStencilStateNone,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(int32_t)
        }
      },
      .descriptorSetLayouts {
        maskDescriptorSetLayout,
        colorsDescriptorSetLayout
      }
    };
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG_H