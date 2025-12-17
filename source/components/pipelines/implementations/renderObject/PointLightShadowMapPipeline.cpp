#include "PointLightShadowMapPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../descriptorSets/LayoutBindings.h"
#include "../../uniformBuffers/UniformBuffer.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../lighting/lights/PointLight.h"

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
        .fragmentShader = "assets/shaders/ShadowCubeMap.frag.spv",
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
                                           const std::shared_ptr<PointLight>& light)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    updateUniformVariables(renderInfo, light);

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

    m_lightUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(glm::vec3));
  }

  void PointLightShadowMapPipeline::createDescriptorSet(VkDescriptorPool descriptorPool)
  {
    m_shadowMapDescriptorSet = std::make_shared<DescriptorSet>(GraphicsPipeline::m_logicalDevice, descriptorPool, LayoutBindings::pointLightShadowMapBindings);
    m_shadowMapDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      const std::vector<VkWriteDescriptorSet> writeDescriptorSets {{
        m_shadowMapUniform->getDescriptorSet(0, descriptorSet, frame),
        m_lightUniform->getDescriptorSet(1, descriptorSet, frame),
      }};

      return writeDescriptorSets;
    });
  }

  void PointLightShadowMapPipeline::updateUniformVariables(const RenderInfo* renderInfo,
                                                           const std::shared_ptr<PointLight>& light) const
  {
    const auto viewProjectionMatrices = light->getLightViewProjectionMatrices();
    m_shadowMapUniform->update(renderInfo->currentFrame, &viewProjectionMatrices);

    const auto lightPosition = light->getPosition();
    m_lightUniform->update(renderInfo->currentFrame, &lightPosition);
  }

  void PointLightShadowMapPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1,
                                                  &m_shadowMapDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }
} // vke