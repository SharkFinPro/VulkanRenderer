#include "PointLightShadowMapPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"

namespace vke {
  PointLightShadowMapPipeline::PointLightShadowMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           std::shared_ptr<RenderPass> renderPass,
                                                           VkDescriptorSetLayout objectDescriptorSetLayout)
    : GraphicsPipeline(logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions{
      .shaders{
        .vertexShader = "assets/shaders/ShadowCubeMap.vert.spv",
      },
      .states{
        .colorBlendState = GraphicsPipelineStates::colorBlendStateShadow,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::multisampleStateNone,
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts{
        objectDescriptorSetLayout
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions, false);
  }

  void PointLightShadowMapPipeline::render(const RenderInfo* renderInfo,
                                           const std::vector<std::shared_ptr<RenderObject>>* objects)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    updateUniformVariables(renderInfo);

    if (objects)
    {
      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(renderInfo->currentFrame, {1.0}, {1.0});

        object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
      }
    }
  }

  void PointLightShadowMapPipeline::updateUniformVariables(const RenderInfo* renderInfo)
  {
    // TODO: Update cube map images uniform
  }
} // vke