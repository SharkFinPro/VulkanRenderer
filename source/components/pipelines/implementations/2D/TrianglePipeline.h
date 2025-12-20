#ifndef VULKANPROJECT_TRIANGLEPIPELINE_H
#define VULKANPROJECT_TRIANGLEPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <glm/mat4x4.hpp>

namespace vke {

  struct Triangle;

  struct TrianglePushConstant {
    float r;
    float g;
    float b;
    float a;
    glm::mat4 transform;
    int screenWidth;
    int screenHeight;
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;
  };

  class TrianglePipeline final : public GraphicsPipeline {
  public:
    TrianglePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     std::shared_ptr<RenderPass> renderPass);

    void render(const RenderInfo* renderInfo,
                const std::vector<Triangle>* triangles);

  private:
    void renderTriangle(const RenderInfo* renderInfo, const Triangle& triangle) const;
  };
} // vke

#endif //VULKANPROJECT_TRIANGLEPIPELINE_H