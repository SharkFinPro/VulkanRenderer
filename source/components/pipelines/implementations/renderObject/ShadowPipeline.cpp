#include "ShadowPipeline.h"
#include "../common/GraphicsPipelineStates.h"
#include "../../../assets/objects/RenderObject.h"
#include "../../../commandBuffer/CommandBuffer.h"

namespace vke {
  ShadowPipeline::ShadowPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                 std::shared_ptr<RenderPass> renderPass,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
    : GraphicsPipeline(std::move(logicalDevice))
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/renderObject/Shadow.vert.spv",
      },
      .states {
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
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = sizeof(glm::mat4)
        }
      },
      .descriptorSetLayouts {
        objectDescriptorSetLayout
      },
      .renderPass = renderPass,
      .colorFormat = VK_FORMAT_UNDEFINED
    };

    createPipeline(graphicsPipelineOptions);
  }

  void ShadowPipeline::render(const RenderInfo* renderInfo,
                              const std::vector<std::shared_ptr<RenderObject>>* objects)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                             sizeof(glm::mat4), &renderInfo->viewMatrix);

    if (objects)
    {
      for (const auto& object : *objects)
      {
        object->updateUniformBuffer(renderInfo->currentFrame, {1.0}, {1.0});

        object->draw(renderInfo->commandBuffer, m_pipelineLayout, renderInfo->currentFrame);
      }
    }
  }
} // vke