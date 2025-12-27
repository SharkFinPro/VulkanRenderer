#ifndef VKE_ELLIPTICALDOTS_H
#define VKE_ELLIPTICALDOTS_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <memory>

namespace vke {

  class DescriptorSet;
  class RenderPass;

  class EllipticalDots final : public GraphicsPipeline {
  public:
    EllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   std::shared_ptr<RenderPass> renderPass,
                   VkDescriptorSetLayout objectDescriptorSetLayout,
                   const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

    void displayGui() override;

  private:
    EllipticalDotsUniform m_ellipticalDotsUBO {
      .shininess = 10.0f,
      .sDiameter = 0.025f,
      .tDiameter = 0.025f,
      .blendFactor = 0.0f
    };

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_ELLIPTICALDOTS_H
