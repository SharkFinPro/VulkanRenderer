#include "MousePickingPipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../components/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../objects/RenderObject.h"

MousePickingPipeline::MousePickingPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const std::shared_ptr<RenderPass>& renderPass,
                                           VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(physicalDevice, logicalDevice), objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  definePushConstants();

  createPipeline(renderPass->getRenderPass());
}

void MousePickingPipeline::render(const RenderInfo* renderInfo,
                                  const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects)
{
  updateUniformVariables(renderInfo);

  vkCmdBindPipeline(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  bindDescriptorSet(renderInfo);

  if (objects)
  {
    for (size_t i = 0; i < objects->size(); ++i)
    {
      MousePickingID id {
        .objectID = objects->at(i).second
      };

      vkCmdPushConstants(renderInfo->commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         sizeof(MousePickingID), &id);

      objects->at(i).first->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      objects->at(i).first->draw(renderInfo->commandBuffer, pipelineLayout, renderInfo->currentFrame, 0);
    }
  }
}

void MousePickingPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/MousePicking.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/MousePicking.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void MousePickingPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void MousePickingPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::multisampleStateNone);
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void MousePickingPipeline::definePushConstants()
{
  constexpr VkPushConstantRange objectID {
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    .offset = 0,
    .size = sizeof(MousePickingID)
  };

  definePushConstantRange(objectID);
}
