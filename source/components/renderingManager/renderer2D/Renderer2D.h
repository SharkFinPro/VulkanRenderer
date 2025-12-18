#ifndef VULKANPROJECT_RENDERER2D_H
#define VULKANPROJECT_RENDERER2D_H

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace vke {
  class Renderer2D {

  private:
    glm::vec4 m_currentFill = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    glm::mat4 m_currentTransform = glm::mat4(1.0f);

    std::vector<glm::mat4> m_transformStack;
  };
} // vke

#endif //VULKANPROJECT_RENDERER2D_H