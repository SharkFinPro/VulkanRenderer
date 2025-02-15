#ifndef VULKANPROJECT_COMPUTEPIPELINE_H
#define VULKANPROJECT_COMPUTEPIPELINE_H

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <chrono>

class UniformBuffer;

struct DeltaTimeUniform {
  float deltaTime = 1.0f;
};

constexpr int PARTICLE_COUNT = 8192;

class DotsPipeline final : public ComputePipeline, public GraphicsPipeline {
public:
  DotsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
               const VkCommandPool& commandPool, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);
  ~DotsPipeline() override;

  void compute(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;
  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, VkExtent2D swapChainExtent);

private:
  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::unique_ptr<UniformBuffer> deltaTimeUniform;

  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool computeDescriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> computeDescriptorSets;

  float dotSpeed;
  std::chrono::time_point<std::chrono::steady_clock> previousTime;

  void loadComputeShaders() override;

  void loadComputeDescriptorSetLayouts() override;

  void loadGraphicsShaders() override;

  void defineStates() override;

  void updateUniformBuffer(uint32_t currentFrame);

  void createUniforms();
  void createShaderStorageBuffers(const VkCommandPool& commandPool, const VkExtent2D& swapChainExtent);

  void createDescriptorSetLayouts();
  void createDescriptorPool();
  void createDescriptorSets();
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
