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
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = sizeof(MVPTransformUniform)
        }
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);

    createVertexBuffer();
  }

  LinePipeline::~LinePipeline()
  {
    Buffers::destroyBuffer(m_logicalDevice, m_stagingBuffer, m_stagingBufferMemory);

    Buffers::destroyBuffer(m_logicalDevice, m_vertexBuffer, m_vertexBufferMemory);
  }

  void LinePipeline::render(const RenderInfo* renderInfo,
                            const VkCommandPool& commandPool,
                            const std::vector<LineVertex>* vertices) const
  {
    if (vertices->empty())
    {
      return;
    }

    bind(renderInfo->commandBuffer);

    const VkDeviceSize bufferSize = sizeof(LineVertex) * vertices->size();

    if (bufferSize > m_maxVertexBufferSize)
    {
      throw std::runtime_error("Vertex data exceeds maximum buffer size");
    }

    m_logicalDevice->doMappedMemoryOperation(m_stagingBufferMemory, [vertices, bufferSize](void* data) {
      memcpy(data, vertices->data(), bufferSize);
    });

    Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), m_stagingBuffer,
                        m_vertexBuffer, bufferSize);

    constexpr VkDeviceSize offsets[] = {0};
    renderInfo->commandBuffer->bindVertexBuffers(0, 1, &m_vertexBuffer, offsets);

    const MVPTransformUniform transformUBO = renderInfo->projectionMatrix * renderInfo->viewMatrix;
    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                             sizeof(MVPTransformUniform), &transformUBO);

    renderInfo->commandBuffer->draw(static_cast<uint32_t>(vertices->size()), 1, 0, 0);
  }

  void LinePipeline::createVertexBuffer()
  {
    Buffers::createBuffer(m_logicalDevice, m_maxVertexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    Buffers::createBuffer(m_logicalDevice, m_maxVertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_stagingBuffer, m_stagingBufferMemory);
  }

} // namespace vke