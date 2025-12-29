#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include <memory>

namespace vke {

  class PipelineManager;

  class Renderer3D {
  public:
    void render(const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void enableGrid();

    void disableGrid();

    [[nodiscard]] bool isGridEnabled() const;

  private:
    bool m_shouldRenderGrid = true;
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H