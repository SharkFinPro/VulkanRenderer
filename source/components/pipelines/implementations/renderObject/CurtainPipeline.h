#ifndef VKE_CURTAINPIPELINE_H
#define VKE_CURTAINPIPELINE_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"

namespace vke {

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class CurtainPipeline final : public GraphicsPipeline {
public:
  CurtainPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  std::shared_ptr<RenderPass> renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

  void displayGui() override;

private:
  CurtainUniform m_curtainUBO {
    .amplitude = 0.1,
    .period = 1,
    .shininess = 10
  };

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
  std::shared_ptr<DescriptorSet> m_curtainDescriptorSet;

  std::shared_ptr<UniformBuffer> m_curtainUniform;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

} // namespace vke

#endif //VKE_CURTAINPIPELINE_H
