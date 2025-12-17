#ifndef VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H
#define VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class DescriptorSet;
  class UniformBuffer;

  class PointLightShadowMapPipeline : public GraphicsPipeline {
  public:
    PointLightShadowMapPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                std::shared_ptr<RenderPass> renderPass,
                                VkDescriptorSetLayout objectDescriptorSetLayout,
                                VkDescriptorPool descriptorPool);

    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<RenderObject>>* objects,
                const std::array<glm::mat4, 6>& lightViewProjectionMatrices);

  private:
    std::shared_ptr<DescriptorSet> m_shadowMapDescriptorSet;

    std::shared_ptr<UniformBuffer> m_shadowMapUniform;

    void createUniforms();

    void createDescriptorSet(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo, const std::array<glm::mat4, 6>& lightViewProjectionMatrices) const;
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHTSHADOWMAPPIPELINE_H