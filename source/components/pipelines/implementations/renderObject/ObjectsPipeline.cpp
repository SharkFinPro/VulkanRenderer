#include "ObjectsPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../logicalDevice/LogicalDevice.h"

namespace vke {

  ObjectsPipeline::ObjectsPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                   std::shared_ptr<RenderPass> renderPass,
                                   const VkDescriptorSetLayout objectDescriptorSetLayout,
                                   const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
    : GraphicsPipeline(std::move(logicalDevice)), m_lightingDescriptorSet(lightingDescriptorSet)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/StandardObject.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/objects.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout,
        m_lightingDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void ObjectsPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1,
                                                  &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }

} // namespace vke