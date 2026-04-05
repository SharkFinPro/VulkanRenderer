#ifndef VULKANPROJECT_SMOKESYSTEM_H
#define VULKANPROJECT_SMOKESYSTEM_H

#include "../../pipelines/implementations/common/Uniforms.h"
#include <vulkan/vulkan_raii.hpp>
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
    SmokeSystem(const std::shared_ptr<LogicalDevice>& logicalDevice,
                vk::CommandPool commandPool,
                vk::DescriptorPool descriptorPool,
                vk::DescriptorSetLayout smokeSystemDescriptorSetLayout,
                glm::vec3 position,
                uint32_t numParticles);

    void update(const RenderInfo* renderInfo);

    [[nodiscard]] uint32_t getNumParticles() const;

    [[nodiscard]] std::shared_ptr<DescriptorSet> getSmokeSystemDescriptorSet() const;

    [[nodiscard]] vk::Buffer getSmokeSystemShaderStorageBuffer(uint32_t currentFrame) const;

    [[nodiscard]] glm::vec3 getPosition() const;

    [[nodiscard]] float getSpeed() const;

    [[nodiscard]] float getSpreadFactor() const;

    [[nodiscard]] float getMaxSpreadDistance() const;

    [[nodiscard]] float getWindStrength() const;

    void setPosition(const glm::vec3& position);

    void setSpeed(float speed);

    void setSpreadFactor(float spreadFactor);

    void setMaxSpreadDistance(float maxSpreadDistance);

    void setWindStrength(float windStrength);

  private:
    std::vector<vk::raii::Buffer> m_shaderStorageBuffers;
    std::vector<vk::raii::DeviceMemory> m_shaderStorageBuffersMemory;
    std::vector<vk::DescriptorBufferInfo> m_shaderStorageBufferInfos;

    std::shared_ptr<DescriptorSet> m_smokeSystemDescriptorSet;

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

    void createUniforms(const std::shared_ptr<LogicalDevice>& logicalDevice);

    void createShaderStorageBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                    const vk::CommandPool& commandPool);

    void uploadShaderStorageBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                    const vk::CommandPool& commandPool,
                                    const std::vector<SmokeParticle>& particles);

    void createDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             vk::DescriptorPool descriptorPool,
                             vk::DescriptorSetLayout smokeSystemDescriptorSetLayout);
  };
} // vke

#endif //VULKANPROJECT_SMOKESYSTEM_H