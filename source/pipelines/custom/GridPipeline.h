#ifndef GRIDPIPELINE_H
#define GRIDPIPELINE_H

#include "../GraphicsPipeline.h"

namespace vke {

class GridPipeline final : public GraphicsPipeline {
public:
  GridPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
               std::shared_ptr<RenderPass> renderPass);

  void render(const RenderInfo* renderInfo);
};

}

#endif //GRIDPIPELINE_H
