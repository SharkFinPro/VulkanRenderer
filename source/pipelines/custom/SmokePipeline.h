#ifndef SMOKEPIPELINE_H
#define SMOKEPIPELINE_H

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <chrono>

class UniformBuffer;

class SmokePipeline final : public ComputePipeline, public GraphicsPipeline {
public:
  SmokePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const VkCommandPool& commandPool,
                const VkRenderPass& renderPass,
                const VkExtent2D& swapChainExtent,
                VkDescriptorPool descriptorPool);

  ~SmokePipeline() override;

  void compute(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

private:
  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::unique_ptr<UniformBuffer> deltaTimeUniform;

  std::unique_ptr<UniformBuffer> transformUniform;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> computeDescriptorSets;

  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;

  float dotSpeed;
  std::chrono::time_point<std::chrono::steady_clock> previousTime;

  uint32_t numParticles = 8192;

  void loadComputeShaders() override;

  void loadComputeDescriptorSetLayouts() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void loadGraphicsShaders() override;

  void defineStates() override;

  void createUniforms();
  void createShaderStorageBuffers(const VkCommandPool& commandPool, const VkExtent2D& swapChainExtent);

  void createDescriptorSetLayouts();

  void createDescriptorSets();

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //SMOKEPIPELINE_H
