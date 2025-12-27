#ifndef VULKANPROJECT_RECTPIPELINE_H
#define VULKANPROJECT_RECTPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <glm/mat4x4.hpp>

namespace vke {

  struct Rect;

  struct RectPushConstant {
    glm::mat4 transform;
    int screenWidth;
    int screenHeight;
    float z;
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
  };

  class RectPipeline final : public GraphicsPipeline {
  public:
    RectPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass);

    void render(const RenderInfo* renderInfo,
                const std::vector<Rect>* rects);

  private:
    void renderRect(const RenderInfo* renderInfo,
                    const Rect& rect) const;
  };
} // vke

#endif //VULKANPROJECT_RECTPIPELINE_H