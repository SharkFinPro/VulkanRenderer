#ifndef VULKANPROJECT_COMPUTEPIPELINE_H
#define VULKANPROJECT_COMPUTEPIPELINE_H

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

class DescriptorSet;
class UniformBuffer;

constexpr int PARTICLE_COUNT = 8192;

class DotsPipeline final : public ComputePipeline, public GraphicsPipeline {
public:
  DotsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const VkCommandPool& commandPool,
               const VkRenderPass& renderPass,
               const VkExtent2D& swapChainExtent,
               VkDescriptorPool descriptorPool);

  ~DotsPipeline() override;

  void compute(const std::shared_ptr<CommandBuffer>& commandBuffer, uint32_t currentFrame) const;

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

private:
  std::vector<VkBuffer> m_shaderStorageBuffers;
  std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;
  std::vector<VkDescriptorBufferInfo> m_shaderStorageBufferInfos;

  std::shared_ptr<DescriptorSet> m_dotsDescriptorSet;

  std::unique_ptr<UniformBuffer> m_deltaTimeUniform;

  float m_dotSpeed;
  std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

  void loadComputeShaders() override;

  void loadComputeDescriptorSetLayouts() override;

  void loadGraphicsShaders() override;

  void defineStates() override;

  void createUniforms();
  void createShaderStorageBuffers(const VkCommandPool& commandPool, const VkExtent2D& swapChainExtent);

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
