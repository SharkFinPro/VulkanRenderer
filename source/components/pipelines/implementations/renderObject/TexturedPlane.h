#ifndef VKE_TEXTUREDPLANE_H
#define VKE_TEXTUREDPLANE_H

#include "../../GraphicsPipeline.h"
#include <memory>

namespace vke {

  class RenderPass;

  class TexturedPlane final : public GraphicsPipeline {
  public:
    TexturedPlane(std::shared_ptr<LogicalDevice> logicalDevice,
                  std::shared_ptr<RenderPass> renderPass,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

  private:
    void render(const RenderInfo* renderInfo,
                const std::vector<std::shared_ptr<RenderObject>>* objects) override;
  };

} // namespace vke

#endif //VKE_TEXTUREDPLANE_H
