#include "PointLightShadowMapPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../descriptorSets/LayoutBindings.h"
#include "../../uniformBuffers/UniformBuffer.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"

namespace vke {
  PointLightShadowMapPipeline::PointLightShadowMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                           std::shared_ptr<RenderPass> renderPass,
                                                           VkDescriptorSetLayout objectDescriptorSetLayout,
                                                           VkDescriptorPool descriptorPool)
    : GraphicsPipeline(logicalDevice)
  {
    createUniforms();

    createDescriptorSet(descriptorPool);

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
        objectDescriptorSetLayout,
        m_shadowMapDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions, false, true);
  }

  void PointLightShadowMapPipeline::render(const RenderInfo* renderInfo,
                                           const std::vector<std::shared_ptr<RenderObject>>* objects,
                                           const std::array<glm::mat4, 6>& lightViewProjectionMatrices)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    updateUniformVariables(renderInfo, lightViewProjectionMatrices);

    if (objects)
    {
      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(renderInfo->currentFrame, {1.0}, {1.0});

        object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
      }
    }
  }

  void PointLightShadowMapPipeline::createUniforms()
  {
    m_shadowMapUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(glm::mat4) * 6);
  }

  void PointLightShadowMapPipeline::createDescriptorSet(VkDescriptorPool descriptorPool)
  {
    m_shadowMapDescriptorSet = std::make_shared<DescriptorSet>(GraphicsPipeline::m_logicalDevice, descriptorPool, LayoutBindings::pointLightShadowMapBindings);
    m_shadowMapDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      const std::vector<VkWriteDescriptorSet> writeDescriptorSets {{
        m_shadowMapUniform->getDescriptorSet(0, descriptorSet, frame)
      }};

      return writeDescriptorSets;
    });
  }

  void PointLightShadowMapPipeline::updateUniformVariables(const RenderInfo* renderInfo,
                                                           const std::array<glm::mat4, 6>& lightViewProjectionMatrices) const
  {
    m_shadowMapUniform->update(renderInfo->currentFrame, &lightViewProjectionMatrices);
  }

  void PointLightShadowMapPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1,
                                                  &m_shadowMapDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }
} // vke