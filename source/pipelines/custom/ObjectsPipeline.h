#ifndef VULKANPROJECT_OBJECTSPIPELINE_H
#define VULKANPROJECT_OBJECTSPIPELINE_H

#include "../GraphicsPipeline.h"
#include <vector>
#include <memory>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;
class LightingDescriptorSet;

class ObjectsPipeline final : public GraphicsPipeline {
public:
  ObjectsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

private:
  std::shared_ptr<LightingDescriptorSet> lightingDescriptorSet;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::shared_ptr<UniformBuffer> lightMetadataUniform;
  std::shared_ptr<UniformBuffer> lightsUniform;
  std::shared_ptr<UniformBuffer> cameraUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};


#endif //VULKANPROJECT_OBJECTSPIPELINE_H
