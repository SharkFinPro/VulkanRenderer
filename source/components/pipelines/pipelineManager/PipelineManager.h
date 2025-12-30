#ifndef VKE_PIPELINEMANAGER_H
#define VKE_PIPELINEMANAGER_H

#include "../implementations/BendyPipeline.h"
#include "../implementations/GridPipeline.h"
#include "../implementations/GuiPipeline.h"
#include "../implementations/LinePipeline.h"
#include "../implementations/SmokePipeline.h"
#include "../implementations/2D/EllipsePipeline.h"
#include "../implementations/2D/FontPipeline.h"
#include "../implementations/2D/RectPipeline.h"
#include "../implementations/2D/TrianglePipeline.h"
#include "../implementations/renderObject/PointLightShadowMapPipeline.h"
#include "../implementations/renderObject/ShadowPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class LightingManager;
  class DotsPipeline;
  class PointLight;
  class Pipeline;
  enum class PipelineType;
  class Renderer;
  class RenderObject;

  class PipelineManager {
  public:
    PipelineManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const std::shared_ptr<Renderer>& renderer,
                    const std::shared_ptr<LightingManager>& lightingManager,
                    const std::shared_ptr<AssetManager>& assetManager,
                    VkDescriptorPool descriptorPool,
                    VkCommandPool commandPool,
                    bool shouldDoDots);

    [[nodiscard]] std::shared_ptr<DotsPipeline> getDotsPipeline();

    void renderGuiPipeline(const RenderInfo* renderInfo) const;

    void renderShadowPipeline(const RenderInfo& renderInfo,
                              const std::vector<std::shared_ptr<RenderObject>>* objects) const;

    void renderPointLightShadowMapPipeline(const RenderInfo& renderInfo,
                                           const std::vector<std::shared_ptr<RenderObject>>* objects,
                                           const std::shared_ptr<PointLight>& pointLight) const;

    void renderBendyPlantPipeline(const RenderInfo& renderInfo,
                                  const std::vector<BendyPlant>* plants) const;

    void renderGridPipeline(const RenderInfo* renderInfo) const;

    void renderRenderObjectPipeline(const RenderInfo* renderInfo,
                                    const std::vector<std::shared_ptr<RenderObject>>* objects,
                                    PipelineType pipelineType) const;

    void renderSmokePipeline(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void computeSmokePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              uint32_t currentFrame,
                              const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void renderLinePipeline(const RenderInfo* renderInfo,
                            const std::vector<LineVertex>* lineVertices) const;

    void renderRectPipeline(const RenderInfo* renderInfo,
                            const std::vector<Rect>* rects) const;

    void renderTrianglePipeline(const RenderInfo* renderInfo,
                                const std::vector<Triangle>* triangles) const;

    void renderEllipsePipeline(const RenderInfo* renderInfo,
                               const std::vector<Ellipse>* ellipses) const;

    void renderFontPipeline(const RenderInfo* renderInfo,
                            const std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Glyph>>>* glyphs,
                            const std::shared_ptr<AssetManager>& assetManager) const;

  private:
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::unique_ptr<GuiPipeline> m_guiPipeline;

    std::shared_ptr<DotsPipeline> m_dotsPipeline;

    std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> m_pipelines;

    std::unique_ptr<SmokePipeline> m_smokePipeline;

    std::unique_ptr<LinePipeline> m_linePipeline;

    std::unique_ptr<BendyPipeline> m_bendyPipeline;

    std::unique_ptr<GridPipeline> m_gridPipeline;

    std::unique_ptr<PointLightShadowMapPipeline> m_pointLightShadowMapPipeline;

    std::unique_ptr<ShadowPipeline> m_shadowPipeline;

    std::unique_ptr<RectPipeline> m_rectPipeline;

    std::unique_ptr<TrianglePipeline> m_trianglePipeline;

    std::unique_ptr<EllipsePipeline> m_ellipsePipeline;

    std::unique_ptr<FontPipeline> m_fontPipeline;

    bool m_shouldDoDots;

    void createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                         const std::shared_ptr<Renderer>& renderer,
                         const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const std::shared_ptr<LightingManager>& lightingManager,
                         VkDescriptorPool descriptorPool);

    void create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                           const std::shared_ptr<Renderer>& renderer,
                           const std::shared_ptr<LogicalDevice>& logicalDevice);

    void createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                     const std::shared_ptr<Renderer>& renderer,
                                     const std::shared_ptr<LogicalDevice>& logicalDevice,
                                     const std::shared_ptr<LightingManager>& lightingManager,
                                     VkDescriptorPool descriptorPool);

    void createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                             const std::shared_ptr<Renderer>& renderer,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<LightingManager>& lightingManager,
                             VkDescriptorPool descriptorPool);
  };

} // namespace vke

#endif //VKE_PIPELINEMANAGER_H
