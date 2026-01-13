#ifndef VKE_SMOKEPIPELINE_H
#define VKE_SMOKEPIPELINE_H

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
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
                  const std::shared_ptr<RenderPass>& renderPass,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet,
                  VkDescriptorSetLayout smokeSystemDescriptorSetLayout);

    void compute(const std::shared_ptr<CommandBuffer>& commandBuffer,
                 uint32_t currentFrame,
                 const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

  private:
    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
  };

} // namespace vke

#endif //VKE_SMOKEPIPELINE_H
