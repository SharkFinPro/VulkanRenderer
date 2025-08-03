#ifndef MAGNIFYWHIRLMOSAIC_H
#define MAGNIFYWHIRLMOSAIC_H

#include "../config/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class MagnifyWhirlMosaicPipeline final : public GraphicsPipeline {
public:
  MagnifyWhirlMosaicPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout objectDescriptorSetLayout);

  void displayGui() override;

private:
  MagnifyWhirlMosaicUniform m_magnifyWhirlMosaicUBO {
    .lensS = 0.5f,
    .lensT = 0.5f,
    .lensRadius = 0.25f,
    .magnification = 1.0f,
    .whirl = 0.0f,
    .mosaic = 0.001f
  };

  std::shared_ptr<DescriptorSet> m_magnifyWhirlMosaicDescriptorSet;

  std::shared_ptr<UniformBuffer> m_cameraUniform;
  std::shared_ptr<UniformBuffer> m_magnifyWhirlMosaicUniform;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //MAGNIFYWHIRLMOSAIC_H
