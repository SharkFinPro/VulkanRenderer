#ifndef VKE_CUBEMAPPIPELINE_H
#define VKE_CUBEMAPPIPELINE_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"

namespace vke {

  class DescriptorSet;
  class RenderPass;
  class UniformBuffer;
  class Texture3D;
  class TextureCubemap;

  class CubeMapPipeline final : public GraphicsPipeline {
  public:
    CubeMapPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                    std::shared_ptr<RenderPass> renderPass,
                    const VkCommandPool& commandPool,
                    VkDescriptorPool descriptorPool,
                    VkDescriptorSetLayout objectDescriptorSetLayout);

    void displayGui() override;

  private:
    CubeMapUniform m_cubeMapUBO {
      .mix = 0,
      .refractionIndex = 1.4,
      .whiteMix = 0.2
    };

    NoiseOptionsUniform m_noiseOptionsUBO {
      .amplitude = 0.0f,
      .frequency = 0.1f
    };

    std::shared_ptr<DescriptorSet> m_cubeMapDescriptorSet;

    std::shared_ptr<UniformBuffer> m_cameraUniform;
    std::shared_ptr<UniformBuffer> m_cubeMapUniform;
    std::shared_ptr<UniformBuffer> m_noiseOptionsUniform;
    std::shared_ptr<Texture3D> m_noiseTexture;

    std::shared_ptr<TextureCubemap> m_reflectUnit;
    std::shared_ptr<TextureCubemap> m_refractUnit;

    void createUniforms(const VkCommandPool& commandPool);

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_CUBEMAPPIPELINE_H
