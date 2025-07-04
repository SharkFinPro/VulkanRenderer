#ifndef BUMPYCURTAIN_H
#define BUMPYCURTAIN_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class PhysicalDevice;
class LogicalDevice;
class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;
class Texture3D;

class BumpyCurtain final : public GraphicsPipeline {
public:
  BumpyCurtain(const std::shared_ptr<PhysicalDevice>& physicalDevice,
               const std::shared_ptr<LogicalDevice>& logicalDevice,
               const std::shared_ptr<RenderPass>& renderPass,
               const VkCommandPool& commandPool,
               VkDescriptorPool descriptorPool,
               VkDescriptorSetLayout objectDescriptorSetLayout);

  ~BumpyCurtain() override;

  void displayGui() override;

private:
  CurtainUniform curtainUBO {
    .amplitude = 0.1,
    .period = 1,
    .shininess = 10
  };

  NoiseOptionsUniform noiseOptionsUBO {
    .amplitude = 0.5f,
    .frequency = 1.0f
  };

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> curtainUniform;
  std::unique_ptr<UniformBuffer> noiseOptionsUniform;
  std::unique_ptr<Texture3D> noiseTexture;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize = 0;

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



#endif //BUMPYCURTAIN_H
