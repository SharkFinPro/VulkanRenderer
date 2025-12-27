#include "PointLightShadowMapPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../descriptorSets/DescriptorSet.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"
#include "../../../lighting/lights/PointLight.h"

namespace vke {
  PointLightShadowMapPipeline::PointLightShadowMapPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                                           std::shared_ptr<RenderPass> renderPass,
                                                           VkDescriptorSetLayout objectDescriptorSetLayout,
                                                           VkDescriptorSetLayout pointLightDescriptorSetLayout)
    : GraphicsPipeline(std::move(logicalDevice))
  {
    const GraphicsPipelineOptions graphicsPipelineOptions{
      .shaders{
        .vertexShader = "assets/shaders/renderObject/ShadowCubeMap.vert.spv",
        .fragmentShader = "assets/shaders/renderObject/ShadowCubeMap.frag.spv"
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
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset = 0,
          .size = sizeof(glm::vec3)
        }
      },
      .descriptorSetLayouts{
        objectDescriptorSetLayout,
        pointLightDescriptorSetLayout
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions, false, true);
  }

  void PointLightShadowMapPipeline::render(const RenderInfo* renderInfo,
                                           const std::vector<std::shared_ptr<RenderObject>>* objects,
                                           const std::shared_ptr<PointLight>& pointLight)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                             sizeof(glm::vec3), &renderInfo->viewPosition);

    pointLight->updateUniform(renderInfo->currentFrame);

    pointLight->bindDescriptorSet(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame);

    if (objects)
    {
      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(renderInfo->currentFrame, {1.0}, {1.0});

        object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame, 0);
      }
    }
  }
} // vke