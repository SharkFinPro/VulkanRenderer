#ifndef VKE_DOTSPIPELINE_H
#define VKE_DOTSPIPELINE_H

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

namespace vke {

  class DescriptorSet;
  class UniformBuffer;

  constexpr int PARTICLE_COUNT = 8192;

  class DotsPipeline final : public ComputePipeline, public GraphicsPipeline {
  public:
    DotsPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 const VkCommandPool& commandPool,
                 const std::shared_ptr<RenderPass>& renderPass,
                 VkDescriptorPool descriptorPool);

    ~DotsPipeline() override;

    void compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                 uint32_t currentFrame) const;

    void render(const RenderInfo* renderInfo);

  private:
    std::vector<VkBuffer> m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;
    std::vector<VkDescriptorBufferInfo> m_shaderStorageBufferInfos;

    std::shared_ptr<DescriptorSet> m_dotsDescriptorSet;

    std::unique_ptr<UniformBuffer> m_deltaTimeUniform;

    float m_dotSpeed = 1000.0f;
    std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

    void createUniforms();
    void createShaderStorageBuffers(const VkCommandPool& commandPool);

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo);
  };

} // namespace vke

#endif //VKE_DOTSPIPELINE_H
