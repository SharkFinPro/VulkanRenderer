#ifndef LINEPIPELINE_H
#define LINEPIPELINE_H

#include "../GraphicsPipeline.h"
#include <vector>
#include <memory>

class RenderPass;
class UniformBuffer;

class LinePipeline final : public GraphicsPipeline {
public:
  LinePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                VkDescriptorPool descriptorPool);
  ~LinePipeline() override;

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout lineDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> transformUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createLineDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();

  void updateUniformVariables(const RenderInfo *renderInfo) override;

  void bindDescriptorSet(const RenderInfo *renderInfo) override;
};



#endif //LINEPIPELINE_H
