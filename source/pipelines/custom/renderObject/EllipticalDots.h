#ifndef ELLIPTICALDOTS_H
#define ELLIPTICALDOTS_H

#include "../config/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <memory>

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class EllipticalDots final : public GraphicsPipeline {
public:
  EllipticalDots(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 const std::shared_ptr<RenderPass>& renderPass,
                 VkDescriptorPool descriptorPool,
                 VkDescriptorSetLayout objectDescriptorSetLayout,
                 const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

  void displayGui() override;

private:
  EllipticalDotsUniform m_ellipticalDotsUBO {
    .shininess = 10.0f,
    .sDiameter = 0.025f,
    .tDiameter = 0.025f,
    .blendFactor = 0.0f
  };

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
  std::shared_ptr<DescriptorSet> m_ellipticalDotsDescriptorSet;

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::shared_ptr<UniformBuffer> m_ellipticalDotsUniform;

  void loadGraphicsDescriptorSetLayouts() override;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //ELLIPTICALDOTS_H
