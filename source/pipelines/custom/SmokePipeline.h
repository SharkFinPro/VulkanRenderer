#ifndef SMOKEPIPELINE_H
#define SMOKEPIPELINE_H

#include "Uniforms.h"
#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

class CommandBuffer;
class UniformBuffer;
struct SmokeParticle;

class SmokePipeline final : public ComputePipeline, public GraphicsPipeline {
public:
  SmokePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
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
  SmokeUniform smokeUBO {
    .spreadFactor = 0.3f,
    .maxSpreadDistance = 7.0f,
    .windStrength = 0.4f
  };

  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::unique_ptr<UniformBuffer> deltaTimeUniform;
  std::unique_ptr<UniformBuffer> transformUniform;
  std::unique_ptr<UniformBuffer> smokeUniform;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;

  int prevNumLights = 0;
  size_t lightsUniformBufferSize = 0;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> computeDescriptorSets;

  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;

  float dotSpeed;
  std::chrono::time_point<std::chrono::steady_clock> previousTime;

  uint32_t numParticles;

  bool ran = false;

  void loadComputeShaders() override;

  void loadComputeDescriptorSetLayouts() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void loadGraphicsShaders() override;

  void defineStates() override;

  void createUniforms();

  void createShaderStorageBuffers(const VkCommandPool& commandPool);

  void uploadShaderStorageBuffers(const VkCommandPool& commandPool, const std::vector<SmokeParticle>& particles);

  void createDescriptorSetLayouts();

  void createDescriptorSets();

  void createDescriptorSet(uint32_t set) const;

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};



#endif //SMOKEPIPELINE_H
