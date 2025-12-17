#ifndef VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H
#define VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {
  class PointLightShadowMapPipeline : public GraphicsPipeline {
  public:
    PointLightShadowMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                std::shared_ptr<RenderPass> renderPass,
                                VkDescriptorSetLayout objectDescriptorSetLayout);

    void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

    void updateUniformVariables(const RenderInfo* renderInfo) override;
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H