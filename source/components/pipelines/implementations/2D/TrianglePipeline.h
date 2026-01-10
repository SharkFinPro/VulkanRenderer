#ifndef VULKANPROJECT_TRIANGLEPIPELINE_H
#define VULKANPROJECT_TRIANGLEPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class TrianglePipeline final : public GraphicsPipeline {
  public:
    TrianglePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                     std::shared_ptr<RenderPass> renderPass);
  };
} // vke

#endif //VULKANPROJECT_TRIANGLEPIPELINE_H