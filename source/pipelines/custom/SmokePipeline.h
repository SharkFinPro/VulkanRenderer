#ifndef SMOKEPIPELINE_H
#define SMOKEPIPELINE_H

#include "config/Uniforms.h"
#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

class DescriptorSet;
class UniformBuffer;
struct SmokeParticle;

class SmokePipeline final : public ComputePipeline, public GraphicsPipeline {
public:
  SmokePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const VkCommandPool& commandPool,
                const VkRenderPass& renderPass,
                VkDescriptorPool descriptorPool,
                glm::vec3 position,
                uint32_t numParticles);

  ~SmokePipeline() override;

  void compute(const std::shared_ptr<CommandBuffer>& commandBuffer, uint32_t currentFrame) const;

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

  void displayGui() override;

private:
  SmokeUniform m_smokeUBO {
    .spreadFactor = 0.3f,
    .maxSpreadDistance = 7.0f,
    .windStrength = 0.4f
  };

  std::vector<VkBuffer> m_shaderStorageBuffers;
  std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;
  std::vector<VkDescriptorBufferInfo> m_shaderStorageBufferInfos;

  std::shared_ptr<DescriptorSet> m_smokeDescriptorSet;
  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

  std::shared_ptr<UniformBuffer> m_deltaTimeUniform;
  std::shared_ptr<UniformBuffer> m_transformUniform;
  std::shared_ptr<UniformBuffer> m_smokeUniform;

  std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
  std::shared_ptr<UniformBuffer> m_lightsUniform;

  int m_prevNumLights = 0;
  size_t m_lightsUniformBufferSize = 0;

  float m_dotSpeed;
  std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

  uint32_t m_numParticles;

  bool m_ran = false;

  void loadComputeShaders() override;

  void loadComputeDescriptorSetLayouts() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void loadGraphicsShaders() override;

  void defineStates() override;

  void createUniforms();

  void createShaderStorageBuffers(const VkCommandPool& commandPool);

  void uploadShaderStorageBuffers(const VkCommandPool& commandPool, const std::vector<SmokeParticle>& particles);

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};



#endif //SMOKEPIPELINE_H
