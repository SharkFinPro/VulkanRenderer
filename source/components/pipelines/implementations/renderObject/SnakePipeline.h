#ifndef VKE_SNAKEPIPELINE_H
#define VKE_SNAKEPIPELINE_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class DescriptorSet;
class RenderPass;

class SnakePipeline final : public GraphicsPipeline {
public:
  SnakePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                std::shared_ptr<RenderPass> renderPass,
                VkDescriptorSetLayout objectDescriptorSetLayout,
                const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

  void displayGui() override;

private:
  SnakeUniform m_snakeUBO {
    .wiggle = 0
  };

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;

  void updateUniformVariables(const RenderInfo* renderInfo) override;
};

} // namespace vke

#endif //VKE_SNAKEPIPELINE_H
