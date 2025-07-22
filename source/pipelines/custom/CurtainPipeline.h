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
                  VkDescriptorSetLayout objectDescriptorSetLayout);

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

  std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
  std::shared_ptr<UniformBuffer> m_lightsUniform;
  std::shared_ptr<UniformBuffer> m_cameraUniform;
  std::shared_ptr<UniformBuffer> m_curtainUniform;

  int m_prevNumLights = 0;

  size_t m_lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

#endif //CURTAINPIPELINE_H
