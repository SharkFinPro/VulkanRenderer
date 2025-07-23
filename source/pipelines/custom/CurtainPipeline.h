#ifndef CURTAINPIPELINE_H
#define CURTAINPIPELINE_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class CurtainPipeline final : public GraphicsPipeline {
public:
  CurtainPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
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

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::shared_ptr<UniformBuffer> m_curtainUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

#endif //CURTAINPIPELINE_H
