#ifndef VULKANPROJECT_RECTPIPELINE_H
#define VULKANPROJECT_RECTPIPELINE_H

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke {

  struct Rect {
    glm::vec4 bounds;
    glm::vec4 color;
    glm::mat4 transform;
  };

  struct RectPushConstant {
    glm::mat4 transform;
    glm::vec2 screenSize;
    glm::vec4 bounds;
    glm::vec4 color;
  };

  class RectPipeline {
  };
} // vke

#endif //VULKANPROJECT_RECTPIPELINE_H