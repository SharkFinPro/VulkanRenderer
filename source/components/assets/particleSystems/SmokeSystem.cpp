#include "SmokeSystem.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../pipelines/implementations/vertexInputs/SmokeParticle.h"
#include "../../../utilities/Buffers.h"
#include <imgui.h>
#include <cstring>
#include <random>

namespace {

  struct ViewProjTransformUniform {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };

}

namespace vke {
  SmokeSystem::SmokeSystem(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const vk::CommandPool commandPool,
                           const vk::DescriptorPool descriptorPool,
                           const vk::DescriptorSetLayout smokeSystemDescriptorSetLayout,
                           const glm::vec3 position,
                           const uint32_t numParticles)
    : m_previousTime(std::chrono::steady_clock::now()),
      m_numParticles(numParticles)
  {
    m_smokeUBO.systemPosition = position;

    createUniforms(logicalDevice);

    createShaderStorageBuffers(logicalDevice, commandPool);

    createDescriptorSet(logicalDevice, descriptorPool, smokeSystemDescriptorSetLayout);
  }

  void SmokeSystem::update(const RenderInfo* renderInfo)
  {
    if (!m_ran)
    {
      m_previousTime = std::chrono::steady_clock::now();
      m_ran = true;
    }

    const auto currentTime = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(currentTime - m_previousTime).count();
    m_previousTime = currentTime;

    const float deltaTimeUBO = m_dotSpeed * dt;

    m_deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);

    const ViewProjTransformUniform transformUBO {
      .view = renderInfo->viewMatrix,
      .proj = renderInfo->getProjectionMatrix()
    };

    m_transformUniform->update(renderInfo->currentFrame, &transformUBO);

    m_smokeUniform->update(renderInfo->currentFrame, &m_smokeUBO);
  }

  uint32_t SmokeSystem::getNumParticles() const
  {
    return m_numParticles;
  }

  std::shared_ptr<DescriptorSet> SmokeSystem::getSmokeSystemDescriptorSet() const
  {
    return m_smokeSystemDescriptorSet;
  }

  vk::Buffer SmokeSystem::getSmokeSystemShaderStorageBuffer(const uint32_t currentFrame) const
  {
    return *m_shaderStorageBuffers[currentFrame];
  }

  glm::vec3 SmokeSystem::getPosition() const
  {
    return m_smokeUBO.systemPosition;
  }

  float SmokeSystem::getSpeed() const
  {
    return m_dotSpeed;
  }

  float SmokeSystem::getSpreadFactor() const
  {
    return m_smokeUBO.spreadFactor;
  }

  float SmokeSystem::getMaxSpreadDistance() const
  {
    return m_smokeUBO.maxSpreadDistance;
  }

  float SmokeSystem::getWindStrength() const
  {
    return m_smokeUBO.windStrength;
  }

  void SmokeSystem::setPosition(const glm::vec3& position)
  {
    m_smokeUBO.systemPosition = position;
  }

  void SmokeSystem::setSpeed(const float speed)
  {
    m_dotSpeed = speed;
  }

  void SmokeSystem::setSpreadFactor(const float spreadFactor)
  {
    m_smokeUBO.spreadFactor = spreadFactor;
  }

  void SmokeSystem::setMaxSpreadDistance(const float maxSpreadDistance)
  {
    m_smokeUBO.maxSpreadDistance = maxSpreadDistance;
  }

  void SmokeSystem::setWindStrength(const float windStrength)
  {
    m_smokeUBO.windStrength = windStrength;
  }

  void SmokeSystem::createUniforms(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    m_deltaTimeUniform = std::make_shared<UniformBuffer>(logicalDevice, sizeof(float));

    m_transformUniform = std::make_shared<UniformBuffer>(logicalDevice, sizeof(ViewProjTransformUniform));

    m_smokeUniform = std::make_shared<UniformBuffer>(logicalDevice, sizeof(SmokeUniform));
  }

  void SmokeSystem::createShaderStorageBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                               const vk::CommandPool& commandPool)
  {
    std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
    std::uniform_real_distribution colorDistribution(0.25f, 1.0f);
    std::uniform_real_distribution largeDistribution(-1000.0f, 1000.0f);

    std::vector<SmokeParticle> particles(m_numParticles);

    double currentTTL = 0;
    const double ttlSpan = 8.0 / static_cast<double>(m_numParticles) * 1.5;

    for (auto& [positionTtl, velocityColor] : particles)
    {
      velocityColor.w = colorDistribution(randomEngine);

      positionTtl = glm::vec4(largeDistribution(randomEngine), 0, 0, currentTTL);

      currentTTL -= currentTTL > -4.0f ? ttlSpan * 4.0f : ttlSpan;
    }

    uploadShaderStorageBuffers(logicalDevice, commandPool, particles);
  }

  void SmokeSystem::uploadShaderStorageBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                               const vk::CommandPool& commandPool,
                                               const std::vector<SmokeParticle>& particles)
  {
    m_shaderStorageBuffers.reserve(logicalDevice->getMaxFramesInFlight());
    m_shaderStorageBuffersMemory.reserve(logicalDevice->getMaxFramesInFlight());

    const vk::DeviceSize bufferSize = sizeof(SmokeParticle) * m_numParticles;

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    Buffers::createBuffer(logicalDevice, bufferSize,
                          vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [particles, bufferSize](void* data) {
      memcpy(data, particles.data(), bufferSize);
    });

    for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
    {
      m_shaderStorageBuffers.emplace_back(nullptr);
      m_shaderStorageBuffersMemory.emplace_back(nullptr);

      Buffers::createBuffer(logicalDevice, bufferSize,
                            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                            vk::MemoryPropertyFlagBits::eDeviceLocal, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);

      Buffers::copyBuffer(logicalDevice, commandPool, logicalDevice->getComputeQueue(),
                          *stagingBuffer, *m_shaderStorageBuffers[i], bufferSize);

      const vk::DescriptorBufferInfo bufferInfo {
        .buffer = *m_shaderStorageBuffers[i],
        .offset = 0,
        .range = bufferSize
      };

      m_shaderStorageBufferInfos.push_back(bufferInfo);
    }
  }

  void SmokeSystem::createDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                        vk::DescriptorPool descriptorPool,
                                        vk::DescriptorSetLayout smokeSystemDescriptorSetLayout)
  {
    m_smokeSystemDescriptorSet = std::make_shared<DescriptorSet>(logicalDevice, descriptorPool, smokeSystemDescriptorSetLayout);
    m_smokeSystemDescriptorSet->updateDescriptorSets([this, logicalDevice](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      constexpr float deltaTimeUBO = 0;

      m_deltaTimeUniform->update(frame, &deltaTimeUBO);
      m_smokeUniform->update(frame, &m_smokeUBO);

      const uint32_t maxFrames = logicalDevice->getMaxFramesInFlight();
      const uint32_t previousFrame = frame == 0 ? maxFrames - 1 : static_cast<uint32_t>(frame - 1);

      const std::vector writeDescriptorSets {{
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
        },
        m_transformUniform->getDescriptorSet(3, descriptorSet, frame),
        m_smokeUniform->getDescriptorSet(4, descriptorSet, frame)
      }};

      return writeDescriptorSets;
    });
  }
} // vke