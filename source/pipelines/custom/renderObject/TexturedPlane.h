#ifndef TEXTUREDPLANE_H
#define TEXTUREDPLANE_H

#include "../../GraphicsPipeline.h"
#include <memory>

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class TexturedPlane final : public GraphicsPipeline {
public:
  TexturedPlane(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                VkDescriptorPool descriptorPool,
                VkDescriptorSetLayout objectDescriptorSetLayout);

private:
  std::shared_ptr<DescriptorSet> m_texturedPlaneDescriptorSet;

  std::shared_ptr<UniformBuffer> m_cameraUniform;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //TEXTUREDPLANE_H
