#ifndef VKE_NOISYELLIPTICALDOTS_H
#define VKE_NOISYELLIPTICALDOTS_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"

namespace vke {

  class DescriptorSet;
  class RenderPass;
  class UniformBuffer;
  class Texture3D;

  class NoisyEllipticalDots final : public GraphicsPipeline {
  public:
    NoisyEllipticalDots(std::shared_ptr<LogicalDevice> logicalDevice,
                        std::shared_ptr<RenderPass> renderPass,
                        const VkCommandPool& commandPool,
                        VkDescriptorPool descriptorPool,
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

    NoiseOptionsUniform m_noiseOptionsUBO {
      .amplitude = 0.5f,
      .frequency = 1.0f
    };

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
    std::shared_ptr<DescriptorSet> m_noisyEllipticalDotsDescriptorSet;

    std::shared_ptr<UniformBuffer> m_ellipticalDotsUniform;
    std::shared_ptr<UniformBuffer> m_noiseOptionsUniform;
    std::shared_ptr<Texture3D> m_noiseTexture;

    void createUniforms(const VkCommandPool& commandPool);

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_NOISYELLIPTICALDOTS_H
