#include "SmokeSystem.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../pipelines/implementations/vertexInputs/SmokeParticle.h"
#include "../../../utilities/Buffers.h"
#include <imgui.h>
#include <random>

namespace vke {

  const std::vector<VkDescriptorSetLayoutBinding> layoutBindings {{
    { // DT
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    { // Last Frame SB
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    },
    { // Current Frame SB
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    },
    { // Transform
      .binding = 3,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    },
    { // Smoke
      .binding = 4,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    }
  }};

  SmokeSystem::SmokeSystem(std::shared_ptr<LogicalDevice> logicalDevice,
                           VkCommandPool commandPool,
                           VkDescriptorPool descriptorPool,
                           const glm::vec3 position,
                           const uint32_t numParticles)
    : m_previousTime(std::chrono::steady_clock::now()),  m_numParticles(numParticles)
  {
    m_smokeUBO.systemPosition = position;

    createUniforms();
    createShaderStorageBuffers(commandPool);

    createDescriptorSets(descriptorPool);
  }

  SmokeSystem::~SmokeSystem()
  {
    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
    {
      Buffers::destroyBuffer(m_logicalDevice, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);
    }
  }

  void SmokeSystem::displayGui()
  {
    ImGui::SliderFloat3("Position", &m_smokeUBO.systemPosition[0], -20, 20);

    ImGui::SliderFloat("Speed", &m_dotSpeed, 0.001f, 10.0f);

    ImGui::SliderFloat("Spread Factor", &m_smokeUBO.spreadFactor, 0.0f, 3.0f);

    ImGui::SliderFloat("Max Spread Distance", &m_smokeUBO.maxSpreadDistance, 0.0f, 20.0f);

    ImGui::SliderFloat("Wind Strength", &m_smokeUBO.windStrength, 0.0f, 3.0f);
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

    const DeltaTimeUniform deltaTimeUBO{m_dotSpeed * dt};

    m_deltaTimeUniform->update(renderInfo->currentFrame, &deltaTimeUBO);

    const ViewProjTransformUniform transformUBO {
      .view = renderInfo->viewMatrix,
      .proj = renderInfo->getProjectionMatrix()
    };

    m_transformUniform->update(renderInfo->currentFrame, &transformUBO);

    m_smokeUniform->update(renderInfo->currentFrame, &m_smokeUBO);
  }

  void SmokeSystem::createUniforms()
  {
    m_deltaTimeUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(DeltaTimeUniform));

    m_transformUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(ViewProjTransformUniform));

    m_smokeUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SmokeUniform));
  }

  void SmokeSystem::createShaderStorageBuffers(const VkCommandPool& commandPool)
  {
    std::default_random_engine randomEngine(static_cast<unsigned int>(time(nullptr)));
    std::uniform_real_distribution<float> colorDistribution(0.25f, 1.0f);
    std::uniform_real_distribution<float> largeDistribution(-1000.0f, 1000.0f);

    std::vector<SmokeParticle> particles(m_numParticles);

    double currentTTL = 0;
    const double ttlSpan = 8.0 / static_cast<double>(m_numParticles) * 1.5;

    for (auto& [positionTtl, velocityColor] : particles)
    {
      velocityColor.w = colorDistribution(randomEngine);

      positionTtl = glm::vec4(largeDistribution(randomEngine), 0, 0, currentTTL);

      currentTTL -= currentTTL > -4.0f ? ttlSpan * 4.0f : ttlSpan;
    }

    uploadShaderStorageBuffers(commandPool, particles);
  }

  void SmokeSystem::uploadShaderStorageBuffers(const VkCommandPool& commandPool,
                                               const std::vector<SmokeParticle>& particles)
  {
    m_shaderStorageBuffers.resize(m_logicalDevice->getMaxFramesInFlight());
    m_shaderStorageBuffersMemory.resize(m_logicalDevice->getMaxFramesInFlight());

    const VkDeviceSize bufferSize = sizeof(SmokeParticle) * m_numParticles;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    Buffers::createBuffer(m_logicalDevice, bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [particles, bufferSize](void* data) {
      memcpy(data, particles.data(), bufferSize);
    });

    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); i++)
    {
      Buffers::createBuffer(m_logicalDevice, bufferSize,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);

      Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getComputeQueue(),
                          stagingBuffer, m_shaderStorageBuffers[i], bufferSize);

      const VkDescriptorBufferInfo bufferInfo {
        .buffer = m_shaderStorageBuffers[i],
        .offset = 0,
        .range = bufferSize
      };

      m_shaderStorageBufferInfos.push_back(bufferInfo);
    }

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void SmokeSystem::createDescriptorSets(VkDescriptorPool descriptorPool)
  {
    m_smokeDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, layoutBindings);
    m_smokeDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      constexpr DeltaTimeUniform deltaTimeUBO{0};

      m_deltaTimeUniform->update(frame, &deltaTimeUBO);
      m_smokeUniform->update(frame, &m_smokeUBO);

      const std::vector<VkWriteDescriptorSet> writeDescriptorSets {{
        m_deltaTimeUniform->getDescriptorSet(0, descriptorSet, frame),
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet,
          .dstBinding = 1,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = &m_shaderStorageBufferInfos[(frame - 1) % m_logicalDevice->getMaxFramesInFlight()]
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet,
          .dstBinding = 2,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = &m_shaderStorageBufferInfos[frame]
        },
        m_transformUniform->getDescriptorSet(3, descriptorSet, frame),
        m_smokeUniform->getDescriptorSet(4, descriptorSet, frame)
      }};

      return writeDescriptorSets;
    });
  }
} // vke