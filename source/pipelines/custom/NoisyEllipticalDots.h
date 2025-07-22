#ifndef NOISYELLIPTICALDOTS_H
#define NOISYELLIPTICALDOTS_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"

class DescriptorSet;
class RenderPass;
class UniformBuffer;
class Texture3D;

class NoisyEllipticalDots final : public GraphicsPipeline {
public:
  NoisyEllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      const std::shared_ptr<RenderPass>& renderPass,
                      const VkCommandPool& commandPool,
                      VkDescriptorPool descriptorPool,
                      VkDescriptorSetLayout objectDescriptorSetLayout);

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

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
  std::shared_ptr<UniformBuffer> m_lightsUniform;
  std::shared_ptr<UniformBuffer> m_cameraUniform;
  std::shared_ptr<UniformBuffer> m_ellipticalDotsUniform;
  std::shared_ptr<UniformBuffer> m_noiseOptionsUniform;
  std::shared_ptr<Texture3D> m_noiseTexture;

  int m_prevNumLights = 0;

  size_t m_lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms(const VkCommandPool& commandPool);

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

#endif //NOISYELLIPTICALDOTS_H
