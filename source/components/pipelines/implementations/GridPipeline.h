#ifndef GRIDPIPELINE_H
#define GRIDPIPELINE_H

#include "../GraphicsPipeline.h"

namespace vke {

  class DescriptorSet;
  class RenderPass;
  class UniformBuffer;

  class GridPipeline final : public GraphicsPipeline {
  public:
    GridPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass,
                 VkDescriptorPool descriptorPool);

    void render(const RenderInfo* renderInfo);

  private:
    std::shared_ptr<DescriptorSet> m_gridDescriptorSet;

    std::shared_ptr<UniformBuffer> m_gridUniform;

    void createUniforms();

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

}

#endif //GRIDPIPELINE_H
