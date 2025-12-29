#include "Renderer3D.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"

namespace vke {
  void Renderer3D::render(const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    // pipelineManager->renderGraphicsPipelines(m_offscreenCommandBuffer, m_offscreenViewportExtent,
    //                                          currentFrame, m_viewPosition, m_viewMatrix, m_shouldRenderGrid);
  }

  void Renderer3D::enableGrid()
  {
    m_shouldRenderGrid = true;
  }

  void Renderer3D::disableGrid()
  {
    m_shouldRenderGrid = false;
  }

  bool Renderer3D::isGridEnabled() const
  {
    return m_shouldRenderGrid;
  }

  void Renderer3D::setCameraParameters(const glm::vec3 position,
                                       const glm::mat4& viewMatrix)
  {
    m_viewPosition = position;
    m_viewMatrix = viewMatrix;
  }
} // vke