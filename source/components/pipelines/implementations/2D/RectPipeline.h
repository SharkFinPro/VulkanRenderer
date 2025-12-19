#ifndef VULKANPROJECT_RECTPIPELINE_H
#define VULKANPROJECT_RECTPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke {

  struct Rect;

  struct RectPushConstant {
    glm::mat4 transform;
    glm::vec2 screenSize;
    glm::vec4 bounds;
    glm::vec4 color;
  };

  class RectPipeline final : public GraphicsPipeline {
  public:
    RectPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 std::shared_ptr<RenderPass> renderPass);

    void render(const RenderInfo* renderInfo,
                const std::vector<Rect>* rects);

  private:
    void renderRect(const RenderInfo* renderInfo, const Rect& rect) const;
  };
} // vke

#endif //VULKANPROJECT_RECTPIPELINE_H