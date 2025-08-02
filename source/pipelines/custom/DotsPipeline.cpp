#include "DotsPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "config/Uniforms.h"
#include "descriptorSets/DescriptorSet.h"
#include "vertexInputs/Particle.h"
#include "../../components/core/commandBuffer/CommandBuffer.h"
#include "../../components/core/logicalDevice/LogicalDevice.h"
#include "../../components/UniformBuffer.h"
#include "../../utilities/Buffers.h"
#include <cmath>
#include <random>
#include <cstring>

const std::vector<VkDescriptorSetLayoutBinding> layoutBindings {{
  {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
  },
  {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  },
  {
    .binding = 2,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
  }
}};

DotsPipeline::DotsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const VkCommandPool& commandPool,
                           const VkRenderPass& renderPass,
                           const VkExtent2D& swapChainExtent,
                           const VkDescriptorPool descriptorPool)
  : ComputePipeline(logicalDevice), GraphicsPipeline(logicalDevice),
    m_dotSpeed(1000.0f), m_previousTime(std::chrono::steady_clock::now())
{
  createUniforms();
  createShaderStorageBuffers(commandPool, swapChainExtent);

  createDescriptorSets(descriptorPool);

  ComputePipeline::createPipeline();
  GraphicsPipeline::createPipeline(renderPass);
}

DotsPipeline::~DotsPipeline()
{
  for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::destroyBuffer(ComputePipeline::m_logicalDevice, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);
  }
}

void DotsPipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer, const uint32_t currentFrame) const
{
  commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipeline);

  commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::m_pipelineLayout, 0,
                                    1, &m_dotsDescriptorSet->getDescriptorSet(currentFrame));

  commandBuffer->dispatch(PARTICLE_COUNT / 256, 1, 1);
}

void DotsPipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, objects);

  constexpr VkDeviceSize offsets[] = {0};
  renderInfo->commandBuffer->bindVertexBuffers(0, 1, &m_shaderStorageBuffers[renderInfo->currentFrame], offsets);

  renderInfo->commandBuffer->draw(PARTICLE_COUNT, 1, 0, 0);
}

void DotsPipeline::loadComputeShaders()
{
  ComputePipeline::createShader("assets/shaders/dots.comp.spv");
}

void DotsPipeline::loadComputeDescriptorSetLayouts()
{
  ComputePipeline::loadDescriptorSetLayout(m_dotsDescriptorSet->getDescriptorSetLayout());
}

void DotsPipeline::loadGraphicsShaders()
{
  GraphicsPipeline::createShader("assets/shaders/dots.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  GraphicsPipeline::createShader("assets/shaders/dots.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void DotsPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateDots);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilStateNone);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStatePointList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(GraphicsPipeline::m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateParticle);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void DotsPipeline::createUniforms()
{
  m_deltaTimeUniform = std::make_unique<UniformBuffer>(ComputePipeline::m_logicalDevice, sizeof(DeltaTimeUniform));
}

void DotsPipeline::createShaderStorageBuffers(const VkCommandPool& commandPool, const VkExtent2D& swapChainExtent)
{
  m_shaderStorageBuffers.resize(ComputePipeline::m_logicalDevice->getMaxFramesInFlight());
  m_shaderStorageBuffersMemory.resize(ComputePipeline::m_logicalDevice->getMaxFramesInFlight());

  std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::uniform_real_distribution<float> largeDistribution(-4.0f, 4.0f);

  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto&[position, velocity, color] : particles)
  {
    const float r = sqrtf(distribution(randomEngine)) * 0.25f;
    const float theta = distribution(randomEngine) * 2.0f * 3.14159265358979323846f;
    const float x = r * std::cos(theta) * static_cast<float>(swapChainExtent.height) / static_cast<float>(swapChainExtent.width);
    const float y = r * std::sin(theta);
    position = glm::vec2(x * largeDistribution(randomEngine), y * largeDistribution(randomEngine));
    velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    color = glm::vec4(distribution(randomEngine), distribution(randomEngine),
                               distribution(randomEngine), 1.0f);
  }

  constexpr VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(ComputePipeline::m_logicalDevice, bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  ComputePipeline::m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [particles](void* data) {
    memcpy(data, particles.data(), bufferSize);
  });

  for (size_t i = 0; i < ComputePipeline::m_logicalDevice->getMaxFramesInFlight(); i++)
  {
    Buffers::createBuffer(ComputePipeline::m_logicalDevice, bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);

    Buffers::copyBuffer(ComputePipeline::m_logicalDevice, commandPool, ComputePipeline::m_logicalDevice->getComputeQueue(),
                        stagingBuffer, m_shaderStorageBuffers[i], bufferSize);

    const VkDescriptorBufferInfo bufferInfo {
      .buffer = m_shaderStorageBuffers[i],
      .offset = 0,
      .range = bufferSize
    };

    m_shaderStorageBufferInfos.push_back(bufferInfo);
  }

  Buffers::destroyBuffer(ComputePipeline::m_logicalDevice, stagingBuffer, stagingBufferMemory);
}

void DotsPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_dotsDescriptorSet = std::make_shared<DescriptorSet>(GraphicsPipeline::m_logicalDevice, descriptorPool, layoutBindings);
  m_dotsDescriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, const size_t frame)
  {
    const std::vector<VkWriteDescriptorSet> writeDescriptorSets {{
      m_deltaTimeUniform->getDescriptorSet(0, descriptorSet, frame),
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &m_shaderStorageBufferInfos[(frame - 1) % ComputePipeline::m_logicalDevice->getMaxFramesInFlight()]
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &m_shaderStorageBufferInfos[frame]
      }
    }};

    return writeDescriptorSets;
  });
}

void DotsPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - m_previousTime).count();
  m_previousTime = currentTime;

  const DeltaTimeUniform deltaTimeUBO{m_dotSpeed * dt};

  m_deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);
}
