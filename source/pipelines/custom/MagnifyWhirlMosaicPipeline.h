#ifndef MAGNIFYWHIRLMOSAIC_H
#define MAGNIFYWHIRLMOSAIC_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class LogicalDevice;
class RenderPass;
class RenderObject;
class UniformBuffer;

class MagnifyWhirlMosaicPipeline final : public GraphicsPipeline {
public:
  MagnifyWhirlMosaicPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout objectDescriptorSetLayout);

  ~MagnifyWhirlMosaicPipeline() override;

  void displayGui() override;

private:
  MagnifyWhirlMosaicUniform magnifyWhirlMosaicUBO {
    .lensS = 0.5f,
    .lensT = 0.5f,
    .lensRadius = 0.25f,
    .magnification = 1.0f,
    .whirl = 0.0f,
    .mosaic = 0.001f
  };

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> magnifyWhirlMosaicUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //MAGNIFYWHIRLMOSAIC_H
