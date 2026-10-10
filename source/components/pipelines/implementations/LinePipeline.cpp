#include "LinePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace {

  struct LinePC {
    glm::mat4 viewProjection;
    glm::vec2 viewportSize;
    float alphaScale;
  };

  vke::GraphicsPipelineOptions createLinePipelineOptions(const std::shared_ptr<vke::LogicalDevice>& logicalDevice,
                                                         const vk::PipelineDepthStencilStateCreateInfo& depthStencilState)
  {
    namespace gps = vke::gps;

    return {
      .shaders {
        .vertexShader = "assets/shaders/Line.vert.spv",
        .fragmentShader = "assets/shaders/Line.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateLine,
        .depthStencilState = depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateLineInstance,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex,
          .offset = 0,
          .size = sizeof(LinePC)
        }
      }
    };
  }

}

namespace vke {

  LinePipeline::LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
    : m_testedPipeline(logicalDevice, createLinePipelineOptions(logicalDevice, gps::depthStencilState)),
      m_onTopPipeline(logicalDevice, createLinePipelineOptions(logicalDevice, gps::depthStencilStateNone)),
      m_hiddenPipeline(logicalDevice, createLinePipelineOptions(logicalDevice, gps::depthStencilStateBehind))
  {
    createVertexBuffers(logicalDevice);
  }

  void LinePipeline::render(const RenderInfo* renderInfo,
                            const LineBatches* lines) const
  {
    // Debug drawing must not take an application down, so lines past a mode's slot are
    // dropped (with one warning) instead of throwing mid-frame.
    std::array<size_t, s_debugDepthModeCount> counts{};

    for (size_t mode = 0; mode < s_debugDepthModeCount; ++mode)
    {
      const auto& batch = (*lines)[mode];

      counts[mode] = std::min(batch.size(), s_maxLines);

      if (counts[mode] < batch.size() && !m_warnedAboutLineLimit)
      {
        m_warnedAboutLineLimit = true;
        std::cerr << "Line limit of " << s_maxLines << " exceeded; extra lines are not drawn" << std::endl;
      }

      if (counts[mode] > 0)
      {
        auto* slot = static_cast<char*>(m_vertexBuffersMapped[renderInfo->currentFrame]) +
                     mode * sizeof(LineInstance) * s_maxLines;

        memcpy(slot, batch.data(), sizeof(LineInstance) * counts[mode]);
      }
    }

    const auto tested = counts[static_cast<size_t>(DebugDepth::tested)];
    const auto xray = counts[static_cast<size_t>(DebugDepth::xray)];

    // The hidden parts go first, so only the scene's own depth decides what is hidden
    if (xray > 0)
    {
      draw(renderInfo, m_hiddenPipeline, DebugDepth::xray, xray, s_hiddenAlphaScale);
    }

    if (tested > 0)
    {
      draw(renderInfo, m_testedPipeline, DebugDepth::tested, tested, 1.0f);
    }

    if (xray > 0)
    {
      draw(renderInfo, m_testedPipeline, DebugDepth::xray, xray, 1.0f);
    }
  }

  void LinePipeline::renderOnTop(const RenderInfo* renderInfo,
                                 const size_t lineCount) const
  {
    if (lineCount > 0)
    {
      draw(renderInfo, m_onTopPipeline, DebugDepth::onTop, std::min(lineCount, s_maxLines), 1.0f);
    }
  }

  void LinePipeline::draw(const RenderInfo* renderInfo,
                          const GraphicsPipeline& pipeline,
                          const DebugDepth slot,
                          const size_t lineCount,
                          const float alphaScale) const
  {
    pipeline.bind(renderInfo->commandBuffer);

    const std::vector<vk::DeviceSize> offsets = {0};
    renderInfo->commandBuffer->bindVertexBuffers(0, { m_vertexBuffers[renderInfo->currentFrame] }, offsets);

    const LinePC pushConstants {
      .viewProjection = renderInfo->getProjectionMatrix() * renderInfo->viewMatrix,
      .viewportSize = { static_cast<float>(renderInfo->extent.width), static_cast<float>(renderInfo->extent.height) },
      .alphaScale = alphaScale
    };

    pipeline.pushConstants<LinePC>(renderInfo->commandBuffer, vk::ShaderStageFlagBits::eVertex, 0, pushConstants);

    renderInfo->commandBuffer->draw(6, static_cast<uint32_t>(lineCount), 0,
                                    static_cast<uint32_t>(static_cast<size_t>(slot) * s_maxLines));
  }

  void LinePipeline::createVertexBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    const auto maxFramesInFlight = logicalDevice->getMaxFramesInFlight();

    m_vertexBuffers.reserve(maxFramesInFlight);
    m_vertexBuffersMemory.reserve(maxFramesInFlight);
    m_vertexBuffersMapped.resize(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
      Buffers::createBuffer(logicalDevice, m_maxVertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                            m_vertexBuffers.emplace_back(nullptr),
                            m_vertexBuffersMemory.emplace_back(nullptr));

      m_vertexBuffersMapped[i] = m_vertexBuffersMemory[i].mapMemory(0, m_maxVertexBufferSize, vk::MemoryMapFlags{});
    }
  }

} // namespace vke