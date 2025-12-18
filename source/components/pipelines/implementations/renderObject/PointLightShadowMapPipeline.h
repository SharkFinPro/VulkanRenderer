#ifndef VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H
#define VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class DescriptorSet;
  class PointLight;
  class UniformBuffer;

  class PointLightShadowMapPipeline : public GraphicsPipeline {
  public:
    PointLightShadowMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                std::shared_ptr<RenderPass> renderPass,
                                VkDescriptorSetLayout objectDescriptorSetLayout,
                                VkDescriptorSetLayout pointLightDescriptorSetLayout);

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<RenderObject>>* objects,
                const std::shared_ptr<PointLight>& pointLight);
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H