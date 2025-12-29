#include "SmokePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "common/Uniforms.h"
#include "vertexInputs/SmokeParticle.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../uniformBuffers/UniformBuffer.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <imgui.h>
#include <random>
#include <cstring>

namespace vke {

  SmokePipeline::SmokePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                               const VkCommandPool& commandPool,
                               std::shared_ptr<RenderPass> renderPass,
                               const VkDescriptorPool descriptorPool,
                               const glm::vec3 position,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
    : ComputePipeline(std::move(logicalDevice)), GraphicsPipeline(std::move(logicalDevice)),
      m_lightingDescriptorSet(lightingDescriptorSet)
  {
    const ComputePipelineOptions computePipelineOptions {
      .shaders {
        .computeShader = "assets/shaders/Smoke.comp.spv",
      },
      // .descriptorSetLayouts {
        // m_smokeDescriptorSet->getDescriptorSetLayout()
      // },
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
        // m_smokeDescriptorSet->getDescriptorSetLayout(),
        m_lightingDescriptorSet->getDescriptorSetLayout()
      },
      .renderPass = renderPass
    };

    GraphicsPipeline::createPipeline(graphicsPipelineOptions);
  }

  void SmokePipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const uint32_t currentFrame) const
  {
    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipeline);

    // TODO: Complete
    // commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipelineLayout, 0,
    //                                   1, &m_smokeDescriptorSet->getDescriptorSet(currentFrame));
    //
    // commandBuffer->dispatch(m_numParticles / 256, 1, 1);
  }

  void SmokePipeline::render(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems)
  {
    // TODO: Complete
    GraphicsPipeline::render(renderInfo, nullptr);

    constexpr VkDeviceSize offsets[] = {0};

    // renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline::m_pipelineLayout, 0,
    //                                               1, &m_smokeDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

    // renderInfo->commandBuffer->bindVertexBuffers(0, 1, &m_shaderStorageBuffers[renderInfo->currentFrame], offsets);

    // renderInfo->commandBuffer->draw(m_numParticles, 1, 0, 0);
  }

  void SmokePipeline::updateUniformVariables(const RenderInfo* renderInfo)
  {

  }

  void SmokePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
  {
    renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline::m_pipelineLayout, 1, 1,
                                                  &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
  }

} // namespace vke