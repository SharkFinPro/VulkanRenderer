#ifndef VULKANPROJECT_SHADOWPIPELINE_H
#define VULKANPROJECT_SHADOWPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {
  class ShadowPipeline final : public GraphicsPipeline {
  public:
    ShadowPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                   std::shared_ptr<RenderPass> renderPass,
                   VkDescriptorSetLayout objectDescriptorSetLayout);

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<RenderObject>>* objects) override;
  };
} // vke

#endif //VULKANPROJECT_SHADOWPIPELINE_H