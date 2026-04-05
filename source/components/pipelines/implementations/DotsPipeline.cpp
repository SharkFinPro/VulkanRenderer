#include "DotsPipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "vertexInputs/Particle.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cmath>
#include <cstring>
#include <random>

namespace vke {

  const std::vector<vk::DescriptorSetLayoutBinding> layoutBindings {{
    {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eCompute,
    },
    {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eCompute
    },
    {
      .binding = 2,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eCompute
    }
  }};

  DotsPipeline::DotsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const vk::raii::CommandPool& commandPool,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const vk::DescriptorPool descriptorPool)
    : m_previousTime(std::chrono::steady_clock::now())
  {
    createUniforms(logicalDevice);

    createShaderStorageBuffers(logicalDevice, commandPool);

    createDescriptorSets(logicalDevice, descriptorPool);

    const ComputePipelineOptions computePipelineOptions {
      .shaders {
        .computeShader = "assets/shaders/dots.comp.spv",
      },
      .descriptorSetLayouts {
        m_dotsDescriptorSet->getDescriptorSetLayout()
      },
    };

    ComputePipeline::createPipeline(logicalDevice, computePipelineOptions);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/dots.vert.spv",
        .fragmentShader = "assets/shaders/dots.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateDots,
        .depthStencilState = GraphicsPipelineStates::depthStencilStateNone,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStatePointList,
        .multisampleState = GraphicsPipelineStates::getMultsampleState(logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateParticle,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .renderPass = renderPass
    };

    GraphicsPipeline::createPipeline(logicalDevice, graphicsPipelineOptions);
  }

  void DotsPipeline::compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                             const uint32_t currentFrame) const
  {
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, ComputePipeline::m_pipeline);

    commandBuffer->bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      ComputePipeline::m_pipelineLayout,
      0,
      { m_dotsDescriptorSet->getDescriptorSet(currentFrame) }
    );

    commandBuffer->dispatch(PARTICLE_COUNT / 256, 1, 1);
  }

  void DotsPipeline::render(const RenderInfo* renderInfo)
  {
    bind(renderInfo->commandBuffer);

    updateUniformVariables(renderInfo);

    renderInfo->commandBuffer->bindVertexBuffers(
      0,
      { m_shaderStorageBuffers[renderInfo->currentFrame] },
      {0}
    );

    renderInfo->commandBuffer->draw(PARTICLE_COUNT, 1, 0, 0);
  }

  void DotsPipeline::createUniforms(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    m_deltaTimeUniform = std::make_unique<UniformBuffer>(logicalDevice, sizeof(float));
  }

  void DotsPipeline::createShaderStorageBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                const vk::raii::CommandPool& commandPool)
  {
    m_shaderStorageBuffers.reserve(logicalDevice->getMaxFramesInFlight());
    m_shaderStorageBuffersMemory.reserve(logicalDevice->getMaxFramesInFlight());

    std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> largeDistribution(-4.0f, 4.0f);

    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto&[position, velocity, color] : particles)
    {
      const float r = sqrtf(distribution(randomEngine)) * 0.25f;
      const float theta = distribution(randomEngine) * 2.0f * 3.14159265358979323846f;
      const float x = r * std::cos(theta);
      const float y = r * std::sin(theta);
      position = glm::vec2(x * largeDistribution(randomEngine), y * largeDistribution(randomEngine));
      velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
      color = glm::vec4(distribution(randomEngine), distribution(randomEngine),
                                 distribution(randomEngine), 1.0f);
    }

    constexpr vk::DeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

    vk::raii::Buffer stagingBuffer{nullptr};
    vk::raii::DeviceMemory stagingBufferMemory{nullptr};

    Buffers::createBuffer(logicalDevice, bufferSize,
                          vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [particles](void* data) {
      memcpy(data, particles.data(), static_cast<size_t>(bufferSize));
    });

    for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
    {
      vk::raii::Buffer buffer{nullptr};
      vk::raii::DeviceMemory memory{nullptr};

      Buffers::createBuffer(logicalDevice, bufferSize,
                            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                            vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, memory);

      Buffers::copyBuffer(logicalDevice, commandPool, logicalDevice->getComputeQueue(),
                          stagingBuffer, buffer, bufferSize);

      const vk::DescriptorBufferInfo bufferInfo {
        .buffer = *buffer,
        .offset = 0,
        .range = bufferSize
      };

      m_shaderStorageBuffers.push_back(std::move(buffer));
      m_shaderStorageBuffersMemory.push_back(std::move(memory));
      m_shaderStorageBufferInfos.push_back(bufferInfo);
    }
  }

  void DotsPipeline::createDescriptorSets(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                          vk::DescriptorPool descriptorPool)
  {
    m_dotsDescriptorSet = std::make_shared<DescriptorSet>(logicalDevice, descriptorPool, layoutBindings);
    m_dotsDescriptorSet->updateDescriptorSets([this, logicalDevice](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      const uint32_t maxFrames = logicalDevice->getMaxFramesInFlight();
      const uint32_t previousFrame = frame == 0 ? maxFrames - 1 : static_cast<uint32_t>(frame - 1);

      const std::vector<vk::WriteDescriptorSet> writeDescriptorSets {{
        m_deltaTimeUniform->getDescriptorSet(0, descriptorSet, frame),
        {
          .dstSet = descriptorSet,
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageBuffer,
          .pBufferInfo = &m_shaderStorageBufferInfos[previousFrame]
        },
        {
          .dstSet = descriptorSet,
          .dstBinding = 2,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageBuffer,
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

    const float deltaTimeUBO = m_dotSpeed * dt;

    m_deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);
  }

} // namespace vke