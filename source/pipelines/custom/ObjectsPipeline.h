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

class ObjectsPipeline final : public GraphicsPipeline {
public:
  ObjectsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                  const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

  ~ObjectsPipeline() override;

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};


#endif //VULKANPROJECT_OBJECTSPIPELINE_H
