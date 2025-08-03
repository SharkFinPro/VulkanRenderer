#ifndef SNAKEPIPELINE_H
#define SNAKEPIPELINE_H

#include "../config/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class SnakePipeline final : public GraphicsPipeline {
public:
  SnakePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                VkDescriptorPool descriptorPool,
                VkDescriptorSetLayout objectDescriptorSetLayout,
                const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

  void displayGui() override;

private:
  SnakeUniform m_snakeUBO {
    .wiggle = 0
  };

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
  std::shared_ptr<DescriptorSet> m_snakeDescriptorSet;

  std::shared_ptr<UniformBuffer> m_snakeUniform;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //SNAKEPIPELINE_H
