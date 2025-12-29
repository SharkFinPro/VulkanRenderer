#ifndef VKE_PIPELINEMANAGER_H
#define VKE_PIPELINEMANAGER_H

#include "../implementations/BendyPipeline.h"
#include "../implementations/GridPipeline.h"
#include "../implementations/LinePipeline.h"
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
  class GuiPipeline;
  class PointLight;
  class Pipeline;
  enum class PipelineType;
  class Renderer;
  class RenderObject;
  class SmokePipeline;

  class PipelineManager {
  public:
    PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                    const std::shared_ptr<Renderer>& renderer,
                    const std::shared_ptr<LightingManager>& lightingManager,
                    VkDescriptorSetLayout objectDescriptorSetLayout,
                    VkDescriptorSetLayout fontDescriptorSetLayout,
                    VkDescriptorPool descriptorPool,
                    VkCommandPool commandPool,
                    bool shouldDoDots);

    std::shared_ptr<SmokePipeline> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                     uint32_t numParticles = 5'000'000);

    void destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem);

    [[nodiscard]] std::shared_ptr<DotsPipeline> getDotsPipeline();

    [[nodiscard]] std::vector<std::shared_ptr<SmokePipeline>>& getSmokeSystems();

    [[nodiscard]] std::shared_ptr<GuiPipeline> getGuiPipeline();

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
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::shared_ptr<RenderPass> m_renderPass;

    std::shared_ptr<LightingManager> m_lightingManager;

    std::shared_ptr<GuiPipeline> m_guiPipeline;
    std::shared_ptr<DotsPipeline> m_dotsPipeline;

    std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> m_pipelines;

    std::vector<std::shared_ptr<SmokePipeline>> m_smokeSystems;

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

    void createPipelines(VkDescriptorSetLayout objectDescriptorSetLayout,
                         VkDescriptorSetLayout fontDescriptorSetLayout,
                         const std::shared_ptr<Renderer>& renderer);
  };

} // namespace vke

#endif //VKE_PIPELINEMANAGER_H
