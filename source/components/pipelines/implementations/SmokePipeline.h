#ifndef VKE_SMOKEPIPELINE_H
#define VKE_SMOKEPIPELINE_H

#include "common/Uniforms.h"
#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <memory>
#include <vector>

namespace vke {

  class DescriptorSet;
  class UniformBuffer;
  struct SmokeParticle;
  class SmokeSystem;

  class SmokePipeline final : public ComputePipeline, public GraphicsPipeline {
  public:
    SmokePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                  const VkCommandPool& commandPool,
                  std::shared_ptr<RenderPass> renderPass,
                  VkDescriptorPool descriptorPool,
                  glm::vec3 position,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

    void compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                 uint32_t currentFrame) const;

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<SmokeSystem>>* systems);

  private:
    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_SMOKEPIPELINE_H
