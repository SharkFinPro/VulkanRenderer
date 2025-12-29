#ifndef VULKANPROJECT_SMOKESYSTEM_H
#define VULKANPROJECT_SMOKESYSTEM_H

#include "../../pipelines/implementations/common/Uniforms.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

namespace vke {

  class DescriptorSet;
  class LogicalDevice;
  struct RenderInfo;
  struct SmokeParticle;
  class UniformBuffer;

  class SmokeSystem {
  public:
    SmokeSystem(std::shared_ptr<LogicalDevice> logicalDevice,
                VkCommandPool commandPool,
                VkDescriptorPool descriptorPool,
                glm::vec3 position,
                uint32_t numParticles);

    ~SmokeSystem();

    void displayGui();

    void update(const RenderInfo* renderInfo);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<VkBuffer> m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;
    std::vector<VkDescriptorBufferInfo> m_shaderStorageBufferInfos;

    std::shared_ptr<DescriptorSet> m_smokeDescriptorSet;

    std::shared_ptr<UniformBuffer> m_deltaTimeUniform;
    std::shared_ptr<UniformBuffer> m_transformUniform;
    std::shared_ptr<UniformBuffer> m_smokeUniform;

    float m_dotSpeed = 0.75f;
    std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

    uint32_t m_numParticles = 0;

    bool m_ran = false;

    SmokeUniform m_smokeUBO {
      .spreadFactor = 0.3f,
      .maxSpreadDistance = 7.0f,
      .windStrength = 0.4f
    };

    void createUniforms();

    void createShaderStorageBuffers(const VkCommandPool& commandPool);

    void uploadShaderStorageBuffers(const VkCommandPool& commandPool,
                                    const std::vector<SmokeParticle>& particles);

    void createDescriptorSets(VkDescriptorPool descriptorPool);
  };
} // vke

#endif //VULKANPROJECT_SMOKESYSTEM_H