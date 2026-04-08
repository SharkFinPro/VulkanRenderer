#include "SmokePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"

namespace vke {

  SmokePipeline::SmokePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet,
                               vk::DescriptorSetLayout smokeSystemDescriptorSetLayout)
  : m_lightingDescriptorSet(lightingDescriptorSet)
  {
    const ComputePipelineOptions computePipelineOptions {
      .shaders {
        .computeShader = "assets/shaders/Smoke.comp.spv",
      },
      .descriptorSetLayouts {
        smokeSystemDescriptorSetLayout
      },
    };

    ComputePipeline::createPipeline(logicalDevice, computePipelineOptions);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Smoke.vert.spv",
        .fragmentShader = "assets/shaders/Smoke.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateSmoke,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStatePointList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateCullBack,
        .vertexInputState = gps::vertexInputStateSmokeParticle,
        .viewportState = gps::viewportState
      },
      .descriptorSetLayouts {
        smokeSystemDescriptorSetLayout,
        m_lightingDescriptorSet->getDescriptorSetLayout()
      }
    };

    GraphicsPipeline::createPipeline(logicalDevice, graphicsPipelineOptions);
  }

  void SmokePipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const uint32_t currentFrame,
                              const std::vector<std::shared_ptr<SmokeSystem>>* systems) const
  {
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, ComputePipeline::m_pipeline);

    for (const auto& system : *systems)
    {
      commandBuffer->bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        ComputePipeline::m_pipelineLayout,
        0,
        { system->getSmokeSystemDescriptorSet()->getDescriptorSet(currentFrame) }
      );

      commandBuffer->dispatch(system->getNumParticles() / 256, 1, 1);
    }
  }

  void SmokePipeline::render(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems) const
  {
    bind(renderInfo->commandBuffer);

    bindDescriptorSet(
      renderInfo->commandBuffer,
      m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      1
    );

    const std::vector<vk::DeviceSize> offsets = {0};

    for (const auto& system : *systems)
    {
      renderInfo->commandBuffer->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        GraphicsPipeline::m_pipelineLayout,
        0,
        { system->getSmokeSystemDescriptorSet()->getDescriptorSet(renderInfo->currentFrame) }
      );

      renderInfo->commandBuffer->bindVertexBuffers(
        0,
        { system->getSmokeSystemShaderStorageBuffer(renderInfo->currentFrame) },
        offsets
      );

      renderInfo->commandBuffer->draw(system->getNumParticles(), 1, 0, 0);
    }
  }

} // namespace vke