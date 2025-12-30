#include "SmokePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <imgui.h>

namespace vke {

  SmokePipeline::SmokePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                               std::shared_ptr<RenderPass> renderPass,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet,
                               VkDescriptorSetLayout smokeSystemDescriptorSetLayout)
    : ComputePipeline(logicalDevice), GraphicsPipeline(std::move(logicalDevice)),
      m_lightingDescriptorSet(lightingDescriptorSet)
  {
    const ComputePipelineOptions computePipelineOptions {
      .shaders {
        .computeShader = "assets/shaders/Smoke.comp.spv",
      },
      .descriptorSetLayouts {
        smokeSystemDescriptorSetLayout
      },
    };

    ComputePipeline::createPipeline(computePipelineOptions);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Smoke.vert.spv",
        .fragmentShader = "assets/shaders/Smoke.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateSmoke,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStatePointList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(GraphicsPipeline::m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateSmokeParticle,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .descriptorSetLayouts {
        smokeSystemDescriptorSetLayout,
        m_lightingDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    GraphicsPipeline::createPipeline(graphicsPipelineOptions);
  }

  void SmokePipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const uint32_t currentFrame,
                              const std::vector<std::shared_ptr<SmokeSystem>>* systems) const
  {
    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipeline);

    for (const auto& system : *systems)
    {
      commandBuffer->bindDescriptorSets(
        VK_PIPELINE_BIND_POINT_COMPUTE,
        ComputePipeline::m_pipelineLayout,
        0,
        1,
        &system->getSmokeSystemDescriptorSet()->getDescriptorSet(currentFrame)
      );

      commandBuffer->dispatch(system->getNumParticles() / 256, 1, 1);
    }
  }

  void SmokePipeline::render(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems)
  {
    GraphicsPipeline::render(renderInfo, nullptr);

    constexpr VkDeviceSize offsets[] = {0};

    for (const auto& system : *systems)
    {
      renderInfo->commandBuffer->bindDescriptorSets(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        GraphicsPipeline::m_pipelineLayout,
        0,
        1,
        &system->getSmokeSystemDescriptorSet()->getDescriptorSet(renderInfo->currentFrame)
      );

      renderInfo->commandBuffer->bindVertexBuffers(
        0,
        1,
        &system->getSmokeSystemShaderStorageBuffer(renderInfo->currentFrame),
        offsets
      );

      renderInfo->commandBuffer->draw(system->getNumParticles(), 1, 0, 0);
    }
  }

  void SmokePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline::m_pipelineLayout, 1, 1,
                                                  &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }

} // namespace vke