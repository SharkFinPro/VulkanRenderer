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
                                VkDescriptorPool descriptorPool);

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<RenderObject>>* objects,
                const std::shared_ptr<PointLight>& light);

  private:
    std::shared_ptr<DescriptorSet> m_shadowMapDescriptorSet;

    std::shared_ptr<UniformBuffer> m_shadowMapUniform;

    std::shared_ptr<UniformBuffer> m_lightUniform;

    void createUniforms();

    void createDescriptorSet(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo,
                                const std::shared_ptr<PointLight>& light) const;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H