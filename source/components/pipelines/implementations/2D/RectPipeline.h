#ifndef VULKANPROJECT_RECTPIPELINE_H
#define VULKANPROJECT_RECTPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {
  class RectPipeline final : public GraphicsPipeline {
  public:
    RectPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass);
  };
} // vke

#endif //VULKANPROJECT_RECTPIPELINE_H