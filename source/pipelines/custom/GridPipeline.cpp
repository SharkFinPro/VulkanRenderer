#include "GridPipeline.h"
#include "../../components/core/commandBuffer/CommandBuffer.h"
#include "../../components/UniformBuffer.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"

namespace vke {
  GridPipeline::GridPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<RenderPass> renderPass,
                             VkDescriptorPool descriptorPool)
    : GraphicsPipeline(logicalDevice)
  {
    createUniforms();

    createDescriptorSets(descriptorPool);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Grid.vert.spv",
        .fragmentShader = "assets/shaders/Grid.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateDots,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        m_gridDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void GridPipeline::render(const RenderInfo* renderInfo)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }

  void GridPipeline::createUniforms()
  {
    m_gridUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(GridUniform));
  }

  void GridPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
  {
    m_gridDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::gridLayoutBindings);
    m_gridDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_gridUniform->getDescriptorSet(0, descriptorSet, frame),
      }};

      return descriptorWrites;
    });
  }

  void GridPipeline::updateUniformVariables(const RenderInfo* renderInfo)
  {
    const GridUniform gridUBO {
      .view = renderInfo->viewMatrix,
      .proj = renderInfo->projectionMatrix
    };

    m_gridUniform->update(renderInfo->currentFrame, &gridUBO);
  }

  void GridPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                  &m_gridDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }
}
