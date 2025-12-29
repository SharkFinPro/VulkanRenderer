#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>

namespace vke {

  class PipelineManager;

  class Renderer3D {
  public:
    void render(const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void enableGrid();

    void disableGrid();

    [[nodiscard]] bool isGridEnabled() const;

    void setCameraParameters(glm::vec3 position,
                             const glm::mat4& viewMatrix);

  private:
    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H