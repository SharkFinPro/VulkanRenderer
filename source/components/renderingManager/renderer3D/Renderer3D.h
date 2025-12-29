#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class LightingManager;
  class LineVertex;
  class LogicalDevice;
  class MousePicker;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;
  class SmokeSystem;
  class Window;

  struct BendyPlant {
    glm::vec3 position = glm::vec3(0.0f);
    int numFins = 21;
    int leafLength = 3;
    float pitch = 77.5;
    float bendStrength = -0.07;
  };

  class Renderer3D {
  public:
    Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
               std::shared_ptr<Window> window,
               const std::shared_ptr<AssetManager>& assetManager,
               VkCommandPool commandPool);

    void renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                          uint32_t currentFrame) const;

    void doMousePicking(uint32_t imageIndex,
                        uint32_t currentFrame);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void createNewFrame();

    void enableGrid();

    void disableGrid();

    [[nodiscard]] bool isGridEnabled() const;

    void setCameraParameters(glm::vec3 position,
                             const glm::mat4& viewMatrix);

    [[nodiscard]] std::shared_ptr<MousePicker> getMousePicker() const;

    [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

    void renderObject(const std::shared_ptr<RenderObject>& renderObject,
                      PipelineType pipelineType,
                      bool* mousePicked = nullptr);

    void renderLine(glm::vec3 start, glm::vec3 end);

    void renderBendyPlant(const BendyPlant& bendyPlant);

    void renderSmokeSystem(const std::shared_ptr<SmokeSystem>& smokeSystem);

    [[nodiscard]] const std::vector<std::shared_ptr<SmokeSystem>>& getSmokeSystems() const;;

  private:
    std::shared_ptr<MousePicker> m_mousePicker;

    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};

    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;

    std::vector<LineVertex> m_lineVerticesToRender;

    std::vector<BendyPlant> m_bendyPlantsToRender;

    std::vector<std::shared_ptr<SmokeSystem>> m_smokeSystemsToRender;

    void renderRenderObjects(const RenderInfo* renderInfo,
                             const std::shared_ptr<PipelineManager>& pipelineManager) const;
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H