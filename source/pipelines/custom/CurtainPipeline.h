#ifndef CURTAINPIPELINE_H
#define CURTAINPIPELINE_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;

class CurtainPipeline final : public GraphicsPipeline {
public:
  CurtainPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

  ~CurtainPipeline() override;

  void displayGui() override;

private:
  CurtainUniform curtainUBO {
    .amplitude = 0.1,
    .period = 1,
    .shininess = 10
  };

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> curtainUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

#endif //CURTAINPIPELINE_H
