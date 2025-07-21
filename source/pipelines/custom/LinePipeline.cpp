#include "LinePipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "config/Uniforms.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../utilities/Buffers.h"
#include <stdexcept>

LinePipeline::LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<RenderPass>& renderPass,
                           VkDescriptorPool descriptorPool)
  : GraphicsPipeline(logicalDevice)
{
  createUniforms();

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());

  createVertexBuffer();
}

LinePipeline::~LinePipeline()
{
  Buffers::destroyBuffer(m_logicalDevice, m_stagingBuffer, m_stagingBufferMemory);

  Buffers::destroyBuffer(m_logicalDevice, m_vertexBuffer, m_vertexBufferMemory);
}

void LinePipeline::render(const RenderInfo* renderInfo, const VkCommandPool& commandPool,
                          const std::vector<LineVertex>& vertices)
{
  if (vertices.empty())
  {
    return;
  }
  
  GraphicsPipeline::render(renderInfo, nullptr);

  const VkDeviceSize bufferSize = sizeof(LineVertex) * vertices.size();

  if (bufferSize > m_maxVertexBufferSize)
  {
    throw std::runtime_error("Vertex data exceeds maximum buffer size");
  }

  m_logicalDevice->doMappedMemoryOperation(m_stagingBufferMemory, [vertices, bufferSize](void* data) {
    memcpy(data, vertices.data(), bufferSize);
  });

  Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), m_stagingBuffer,
                      m_vertexBuffer, bufferSize);

  constexpr VkDeviceSize offsets[] = {0};
  renderInfo->commandBuffer->bindVertexBuffers(0, 1, &m_vertexBuffer, offsets);

  renderInfo->commandBuffer->draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}

void LinePipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Line.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void LinePipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_lineDescriptorSet->getDescriptorSetLayout());
}

void LinePipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateLineList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateLineVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void LinePipeline::createUniforms()
{
  m_transformUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(TransformUniform));
}

void LinePipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_lineDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::lineLayoutBindings);
  m_lineDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_transformUniform->getDescriptorSet(0, descriptorSet, frame),
    }};

    return descriptorWrites;
  });
}

void LinePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const TransformUniform transformUBO {
    // .model = createModelMatrix(),
    .model = glm::mat4(1.0f),
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->projectionMatrix
  };

  m_transformUniform->update(renderInfo->currentFrame, &transformUBO);
}

void LinePipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_lineDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
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
