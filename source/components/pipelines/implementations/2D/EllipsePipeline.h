#ifndef VULKANPROJECT_ELLIPSEPIPELINE_H
#define VULKANPROJECT_ELLIPSEPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class EllipsePipeline : public GraphicsPipeline {
  public:
    EllipsePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass);
  };
} // vke

#endif //VULKANPROJECT_ELLIPSEPIPELINE_H