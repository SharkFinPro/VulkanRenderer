#include "LinePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <algorithm>
#include <iostream>

namespace {

  struct LinePC {
    glm::mat4 viewProjection;
    glm::vec2 viewportSize;
  };

}

namespace vke {

  LinePipeline::LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Line.vert.spv",
        .fragmentShader = "assets/shaders/Line.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateLine,
        .depthStencilState = gps::depthStencilState,
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

    createPipeline(logicalDevice, graphicsPipelineOptions);

    createVertexBuffers(logicalDevice);
  }

  void LinePipeline::render(const RenderInfo* renderInfo,
                            const std::vector<LineInstance>* lines) const
  {
    if (lines->empty())
    {
      return;
    }

    // Debug drawing must not take an application down, so lines past the buffer's capacity are
    // dropped (with one warning) instead of throwing mid-frame.
    const size_t lineCount = std::min(lines->size(), s_maxLines);

    if (lineCount < lines->size() && !m_warnedAboutLineLimit)
    {
      m_warnedAboutLineLimit = true;
      std::cerr << "Line limit of " << s_maxLines << " exceeded; extra lines are not drawn" << std::endl;
    }

    bind(renderInfo->commandBuffer);

    memcpy(m_vertexBuffersMapped[renderInfo->currentFrame], lines->data(), sizeof(LineInstance) * lineCount);

    const std::vector<vk::DeviceSize> offsets = {0};
    renderInfo->commandBuffer->bindVertexBuffers(0, { m_vertexBuffers[renderInfo->currentFrame] }, offsets);

    const LinePC pushConstants {
      .viewProjection = renderInfo->getProjectionMatrix() * renderInfo->viewMatrix,
      .viewportSize = { static_cast<float>(renderInfo->extent.width), static_cast<float>(renderInfo->extent.height) }
    };

    renderInfo->commandBuffer->pushConstants<LinePC>(
      m_pipelineLayout,
      vk::ShaderStageFlagBits::eVertex,
      0,
      pushConstants
    );

    renderInfo->commandBuffer->draw(6, static_cast<uint32_t>(lineCount), 0, 0);
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