#include "LinePipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "common/Uniforms.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <stdexcept>

namespace vke {

  LinePipeline::LinePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass)
    : GraphicsPipeline(std::move(logicalDevice))
  {
    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Line.vert.spv",
        .fragmentShader = "assets/shaders/Line.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendState,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateLineList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateLineVertex,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex,
          .offset = 0,
          .size = sizeof(MVPTransformUniform)
        }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);

    createVertexBuffer();
  }

  void LinePipeline::render(const RenderInfo* renderInfo,
                            const vk::raii::CommandPool& commandPool,
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

    m_logicalDevice->doMappedMemoryOperation(m_stagingBufferMemory, [vertices, bufferSize](void* data) {
      memcpy(data, vertices->data(), bufferSize);
    });

    Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), m_stagingBuffer,
                        m_vertexBuffer, bufferSize);

    const std::vector<vk::DeviceSize> offsets = {0};
    renderInfo->commandBuffer->bindVertexBuffers(0, { m_vertexBuffer }, offsets);

    const MVPTransformUniform transformUBO = renderInfo->projectionMatrix * renderInfo->viewMatrix;

    renderInfo->commandBuffer->pushConstants<MVPTransformUniform>(
      m_pipelineLayout,
      vk::ShaderStageFlagBits::eVertex,
      0,
      transformUBO
    );

    renderInfo->commandBuffer->draw(static_cast<uint32_t>(vertices->size()), 1, 0, 0);
  }

  void LinePipeline::createVertexBuffer()
  {
    Buffers::createBuffer(m_logicalDevice, m_maxVertexBufferSize,
                          vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                          vk::MemoryPropertyFlagBits::eDeviceLocal, m_vertexBuffer, m_vertexBufferMemory);

    Buffers::createBuffer(m_logicalDevice, m_maxVertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          m_stagingBuffer, m_stagingBufferMemory);
  }

} // namespace vke