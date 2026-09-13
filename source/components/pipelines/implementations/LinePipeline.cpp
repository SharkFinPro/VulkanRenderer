#include "LinePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <stdexcept>

namespace {

  using MVPTransformPC = glm::mat4;

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
        .colorBlendState = gps::colorBlendState,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateLineList,
        .multisampleState = gps::getMultsampleState(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateLineVertex,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex,
          .offset = 0,
          .size = sizeof(MVPTransformPC)
        }
      }
    };

    createPipeline(logicalDevice, graphicsPipelineOptions);

    createVertexBuffers(logicalDevice);
  }

  void LinePipeline::render(const RenderInfo* renderInfo,
                            const std::vector<LineVertex>* vertices) const
  {
    if (vertices->empty())
    {
      return;
    }

    bind(renderInfo->commandBuffer);

    const vk::DeviceSize bufferSize = sizeof(LineVertex) * vertices->size();

    if (bufferSize > m_maxVertexBufferSize)
    {
      throw std::runtime_error("Vertex data exceeds maximum buffer size");
    }

    memcpy(m_vertexBuffersMapped[renderInfo->currentFrame], vertices->data(), bufferSize);

    const std::vector<vk::DeviceSize> offsets = {0};
    renderInfo->commandBuffer->bindVertexBuffers(0, { m_vertexBuffers[renderInfo->currentFrame] }, offsets);

    const MVPTransformPC transformUBO = renderInfo->getProjectionMatrix() * renderInfo->viewMatrix;

    renderInfo->commandBuffer->pushConstants<MVPTransformPC>(
      m_pipelineLayout,
      vk::ShaderStageFlagBits::eVertex,
      0,
      transformUBO
    );

    renderInfo->commandBuffer->draw(static_cast<uint32_t>(vertices->size()), 1, 0, 0);
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