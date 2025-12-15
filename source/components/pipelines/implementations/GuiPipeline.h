#ifndef VKE_GUIPIPELINE_H
#define VKE_GUIPIPELINE_H

#include "../GraphicsPipeline.h"

namespace vke {

class RenderPass;

class GuiPipeline final : public GraphicsPipeline {
public:
  GuiPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
              std::shared_ptr<RenderPass> renderPass);

  void render(const RenderInfo* renderInfo);
};

} // namespace vke

#endif //VKE_GUIPIPELINE_H
