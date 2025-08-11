#ifndef VULKANPROJECT_GUIPIPELINE_H
#define VULKANPROJECT_GUIPIPELINE_H

#include "../GraphicsPipeline.h"

class RenderPass;

class GuiPipeline final : public GraphicsPipeline {
public:
  GuiPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const std::shared_ptr<RenderPass>& renderPass);

  void render(const RenderInfo* renderInfo);
};


#endif //VULKANPROJECT_GUIPIPELINE_H
