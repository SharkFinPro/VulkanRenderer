#ifndef CUBEMAPPIPELINE_H
#define CUBEMAPPIPELINE_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Texture3D;
class TextureCubemap;

class CubeMapPipeline final : public GraphicsPipeline {
public:
  CubeMapPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                  const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  const VkCommandPool& commandPool,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

  ~CubeMapPipeline() override;

  void displayGui() override;

private:
  CubeMapUniform cubeMapUBO {
    .mix = 0,
    .refractionIndex = 1.4,
    .whiteMix = 0.2
  };

  NoiseOptionsUniform noiseOptionsUBO {
    .amplitude = 0.0f,
    .frequency = 0.1f
  };

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> cubeMapUniform;
  std::unique_ptr<UniformBuffer> noiseOptionsUniform;
  std::unique_ptr<Texture3D> noiseTexture;

  std::unique_ptr<TextureCubemap> reflectUnit;
  std::unique_ptr<TextureCubemap> refractUnit;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms(const VkCommandPool& commandPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //CUBEMAPPIPELINE_H
