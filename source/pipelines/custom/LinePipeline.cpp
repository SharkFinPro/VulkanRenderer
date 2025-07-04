#include "LinePipeline.h"
#include "GraphicsPipelineStates.h"
#include "Uniforms.h"
#include "../RenderPass.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../utilities/Buffers.h"
#include <stdexcept>

LinePipeline::LinePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<RenderPass>& renderPass,
                           VkDescriptorPool descriptorPool)
  : GraphicsPipeline(physicalDevice, logicalDevice), descriptorPool(descriptorPool)
{
  createUniforms();

  createLineDescriptorSetLayout();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());

  createVertexBuffer();
}

LinePipeline::~LinePipeline()
{
  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);

  Buffers::destroyBuffer(logicalDevice, vertexBuffer, vertexBufferMemory);

  logicalDevice->destroyDescriptorSetLayout(lineDescriptorSetLayout);
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

  if (bufferSize > maxVertexBufferSize)
  {
    throw std::runtime_error("Vertex data exceeds maximum buffer size");
  }

  logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [vertices, bufferSize](void* data) {
    memcpy(data, vertices.data(), bufferSize);
  });

  Buffers::copyBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), stagingBuffer,
                      vertexBuffer, bufferSize);

  constexpr VkDeviceSize offsets[] = {0};
  renderInfo->commandBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);

  renderInfo->commandBuffer->draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}

void LinePipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Line.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void LinePipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(lineDescriptorSetLayout);
}

void LinePipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateLineList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateLineVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void LinePipeline::createLineDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 1> lineBindings {
    transformLayout
  };

  const VkDescriptorSetLayoutCreateInfo lineLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(lineBindings.size()),
    .pBindings = lineBindings.data()
  };

  lineDescriptorSetLayout = logicalDevice->createDescriptorSetLayout(lineLayoutCreateInfo);
}

void LinePipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), lineDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(logicalDevice->getMaxFramesInFlight());
  logicalDevice->allocateDescriptorSets(allocateInfo, descriptorSets.data());

  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::array<VkWriteDescriptorSet, 1> descriptorWrites{{
      transformUniform->getDescriptorSet(0, descriptorSets[i], i),
    }};

    logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

void LinePipeline::createUniforms()
{
  transformUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(TransformUniform));
}

void LinePipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const TransformUniform transformUBO {
    // .model = createModelMatrix(),
    .model = glm::mat4(1.0f),
    .view = renderInfo->viewMatrix,
    .proj = renderInfo->projectionMatrix
  };

  transformUniform->update(renderInfo->currentFrame, &transformUBO);
}

void LinePipeline::bindDescriptorSet(const RenderInfo *renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                                &descriptorSets[renderInfo->currentFrame]);
}

void LinePipeline::createVertexBuffer()
{
  Buffers::createBuffer(logicalDevice, maxVertexBufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  Buffers::createBuffer(logicalDevice, maxVertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);
}
