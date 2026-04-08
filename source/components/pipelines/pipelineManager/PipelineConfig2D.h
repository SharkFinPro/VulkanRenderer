#ifndef VULKANPROJECT_PIPELINECONFIG2D_H
#define VULKANPROJECT_PIPELINECONFIG2D_H

#include "../implementations/common/GraphicsPipelineStates.h"
#include "../GraphicsPipeline.h"
#include "../../renderingManager/renderer2D/Primitives2D.h"

namespace vke::PipelineConfig {

  inline GraphicsPipelineOptions createRectPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Rect.vert.spv",
        .fragmentShader = "assets/shaders/2D/Rect.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateTransparent,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(Rect::PushConstant)
        }
      }
    };
  }

  inline GraphicsPipelineOptions createTrianglePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Triangle.vert.spv",
        .fragmentShader = "assets/shaders/2D/Triangle.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateTransparent,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(Triangle::PushConstant)
        }
      }
    };
  }

  inline GraphicsPipelineOptions createEllipsePipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Ellipse.vert.spv",
        .fragmentShader = "assets/shaders/2D/Ellipse.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateTransparent,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(Ellipse::PushConstant)
        }
      }
    };
  }

  inline GraphicsPipelineOptions createFontPipelineOptions(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           vk::DescriptorSetLayout fontDescriptorSetLayout)
  {
    return {
      .shaders {
        .vertexShader = "assets/shaders/2D/Font.vert.spv",
        .fragmentShader = "assets/shaders/2D/Font.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateTransparent,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          .offset = 0,
          .size = sizeof(Glyph::PushConstant)
        }
      },
      .descriptorSetLayouts {
        fontDescriptorSetLayout
      }
    };
  }

} // vke::PipelineConfig

#endif //VULKANPROJECT_PIPELINECONFIG2D_H