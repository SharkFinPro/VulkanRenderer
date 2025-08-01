#ifndef BENDYPIPELINE_H
#define BENDYPIPELINE_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"
#include <glm/vec3.hpp>

class UniformBuffer;
class DescriptorSet;
class RenderPass;

class BendyPipeline final : public GraphicsPipeline {
public:
  BendyPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                const VkCommandPool& commandPool,
                VkDescriptorPool descriptorPool);

  void render(const RenderInfo* renderInfo);

private:
  BendyUniform m_bendyUBO {
    .pitch = 70,
    .bendStrength = -0.07
  };

  std::shared_ptr<UniformBuffer> m_transformUniform;
  std::shared_ptr<UniformBuffer> m_bendyUniform;

  std::shared_ptr<DescriptorSet> m_BendyPipelineDescriptorSet;

  glm::vec3 m_position = glm::vec3(0, -2, 3);

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms(const VkCommandPool& commandPool);

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //BENDYPIPELINE_H
