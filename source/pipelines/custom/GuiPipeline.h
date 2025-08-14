#ifndef VULKANPROJECT_GUIPIPELINE_H
#define VULKANPROJECT_GUIPIPELINE_H

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

#endif //VULKANPROJECT_GUIPIPELINE_H
