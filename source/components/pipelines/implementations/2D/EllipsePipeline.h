#ifndef VULKANPROJECT_ELLIPSEPIPELINE_H
#define VULKANPROJECT_ELLIPSEPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <glm/mat4x4.hpp>

namespace vke {

  struct Ellipse;

  struct EllipsePushConstant {
    glm::mat4 transform;
    int screenWidth;
    int screenHeight;
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
  };

  class EllipsePipeline : public GraphicsPipeline {
  public:
    EllipsePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 std::shared_ptr<RenderPass> renderPass);

    void render(const RenderInfo* renderInfo,
                const std::vector<Ellipse>* ellipses);

  private:
    void renderEllipse(const RenderInfo* renderInfo, const Ellipse& ellipse) const;
  };
} // vke

#endif //VULKANPROJECT_ELLIPSEPIPELINE_H