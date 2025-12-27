#ifndef VKE_OBJECTHIGHLIGHTPIPELINE_H
#define VKE_OBJECTHIGHLIGHTPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class RenderPass;

  class ObjectHighlightPipeline final : public GraphicsPipeline {
  public:
    ObjectHighlightPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            std::shared_ptr<RenderPass> renderPass,
                            VkDescriptorSetLayout objectDescriptorSetLayout);

    void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;
  };

} // namespace vke

#endif //VKE_OBJECTHIGHLIGHTPIPELINE_H
