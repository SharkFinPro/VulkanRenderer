#ifndef NOISYELLIPTICALDOTS_H
#define NOISYELLIPTICALDOTS_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"

class LightingDescriptorSet;
class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;
class Texture3D;

class NoisyEllipticalDots final : public GraphicsPipeline {
public:
  NoisyEllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      const std::shared_ptr<RenderPass>& renderPass,
                      const VkCommandPool& commandPool,
                      VkDescriptorPool descriptorPool,
                      VkDescriptorSetLayout objectDescriptorSetLayout);

  ~NoisyEllipticalDots() override;

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

  std::shared_ptr<LightingDescriptorSet> m_lightingDescriptorSet;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets;

  VkDescriptorSetLayout m_globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> m_lightMetadataUniform;
  std::unique_ptr<UniformBuffer> m_lightsUniform;
  std::unique_ptr<UniformBuffer> m_cameraUniform;
  std::unique_ptr<UniformBuffer> m_ellipticalDotsUniform;
  std::unique_ptr<UniformBuffer> m_noiseOptionsUniform;
  std::unique_ptr<Texture3D> m_noiseTexture;

  int m_prevNumLights = 0;

  size_t m_lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms(const VkCommandPool& commandPool);

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

#endif //NOISYELLIPTICALDOTS_H
