#include "CrossesPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

CrossesPipeline::CrossesPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkDescriptorPool descriptorPool,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout,
                                 const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
: GraphicsPipeline(logicalDevice),
  m_objectDescriptorSetLayout(objectDescriptorSetLayout),
  m_lightingDescriptorSet(lightingDescriptorSet)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void CrossesPipeline::displayGui()
{
  ImGui::Begin("Crosses");

  ImGui::SliderInt("Level", &m_crossesUBO.level, 0, 3);

  ImGui::SliderFloat("Quantize", &m_crossesUBO.quantize, 2.0f, 50.0f);

  ImGui::SliderFloat("Size", &m_crossesUBO.size, 0.0001f, 0.1f);

  ImGui::SliderFloat("Shininess", &m_crossesUBO.shininess, 2.0f, 50.0f);

  ImGui::End();

  ImGui::Begin("Chroma Depth");

  ImGui::Checkbox("Use Chroma Depth", &m_chromaDepthUBO.use);

  ImGui::SliderFloat("Blue Depth", &m_chromaDepthUBO.blueDepth, 0.0f, 50.0f);

  ImGui::SliderFloat("Red Depth", &m_chromaDepthUBO.redDepth, 0.0f, 50.0f);

  ImGui::End();
}

void CrossesPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Crosses.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Crosses.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
  createShader("assets/shaders/Crosses.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void CrossesPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_crossesDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
}

void CrossesPipeline::defineStates()
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

void CrossesPipeline::createUniforms()
{
  m_crossesUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CrossesUniform));

  m_chromaDepthUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(ChromaDepthUniform));
}

void CrossesPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_crossesDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::crossesLayoutBindings);
  m_crossesDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_crossesUniform->getDescriptorSet(4, descriptorSet, frame),
      m_chromaDepthUniform->getDescriptorSet(6, descriptorSet, frame)
    }};

    return descriptorWrites;
  });
}

void CrossesPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  m_cameraUBO.position = renderInfo->viewPosition;

  m_chromaDepthUniform->update(renderInfo->currentFrame, &m_chromaDepthUBO);

  m_crossesUniform->update(renderInfo->currentFrame, &m_crossesUBO);
}

void CrossesPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_crossesDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
