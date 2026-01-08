#include "MousePickingPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../common/Uniforms.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"

namespace vke {

  MousePickingPipeline::MousePickingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                             std::shared_ptr<RenderPass> renderPass,
                                             VkDescriptorSetLayout objectDescriptorSetLayout)
    : GraphicsPipeline(std::move(logicalDevice))
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
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

    createPipeline(graphicsPipelineOptions);
  }

  void MousePickingPipeline::render(const RenderInfo* renderInfo,
                                    const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects)
  {
    updateUniformVariables(renderInfo);

    renderInfo->commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    bindDescriptorSet(renderInfo);

    for (const auto& [first, second] : *objects)
    {
      renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                               sizeof(uint32_t), &second);

      first->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      first->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
    }
  }

} // namespace vke