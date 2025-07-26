#include "ObjectsPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include <imgui.h>

ObjectsPipeline::ObjectsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout,
                                 const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
  : GraphicsPipeline(logicalDevice),
    m_lightingDescriptorSet(lightingDescriptorSet),
    m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createPipeline(renderPass->getRenderPass());
}

void ObjectsPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/objects.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ObjectsPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
}

void ObjectsPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void ObjectsPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
