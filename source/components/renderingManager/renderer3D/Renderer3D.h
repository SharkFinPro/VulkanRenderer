#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class BendyPlant;
  class LineVertex;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;

  class Renderer3D {
  public:
    void render(const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void createNewFrame();

    void enableGrid();

    void disableGrid();

    [[nodiscard]] bool isGridEnabled() const;

    void setCameraParameters(glm::vec3 position,
                             const glm::mat4& viewMatrix);

    [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

    void renderObject(const std::shared_ptr<RenderObject>& renderObject,
                      PipelineType pipelineType,
                      bool* mousePicked = nullptr);

    void renderLine(glm::vec3 start, glm::vec3 end);

    void renderBendyPlant(const BendyPlant& bendyPlant) const;

  private:
    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};

    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;

    std::vector<LineVertex> m_lineVerticesToRender;

    void renderRenderObjects(const RenderInfo& renderInfo) const;

    void renderSmokeSystems(const RenderInfo& renderInfo) const;
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H