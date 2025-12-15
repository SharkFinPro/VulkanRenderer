#include "MousePickingPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../common/Uniforms.h"
#include "../../../RenderPass.h"
#include "../../../core/commandBuffer/CommandBuffer.h"
#include "../../../core/logicalDevice/LogicalDevice.h"
#include "../../../objects/RenderObject.h"

namespace vke {

MousePickingPipeline::MousePickingPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           std::shared_ptr<RenderPass> renderPass,
                                           VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice)
{
  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/MousePicking.vert.spv",
      .fragmentShader = "assets/shaders/MousePicking.frag.spv"
    },
    .states {
      .colorBlendState = GraphicsPipelineStates::colorBlendState,
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
        .size = sizeof(MousePickingID)
      }
    },
    .descriptorSetLayouts {
      objectDescriptorSetLayout
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
}

void MousePickingPipeline::render(const RenderInfo* renderInfo,
                                  const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects)
{
  updateUniformVariables(renderInfo);

  renderInfo->commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

  bindDescriptorSet(renderInfo);

  if (objects)
  {
    for (size_t i = 0; i < objects->size(); ++i)
    {
      MousePickingID id {
        .objectID = objects->at(i).second
      };

      renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                               sizeof(MousePickingID), &id);

      objects->at(i).first->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      objects->at(i).first->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
    }
  }
}

} // namespace vke