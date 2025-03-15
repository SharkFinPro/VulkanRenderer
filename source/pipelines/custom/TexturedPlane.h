#ifndef TEXTUREDPLANE_H
#define TEXTUREDPLANE_H

#include "../GraphicsPipeline.h"
#include <vector>
#include <memory>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;

class TexturedPlane final : public GraphicsPipeline {
public:
  TexturedPlane(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                VkDescriptorPool descriptorPool,
                VkDescriptorSetLayout objectDescriptorSetLayout);

  ~TexturedPlane() override;

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> cameraUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();
};



#endif //TEXTUREDPLANE_H
