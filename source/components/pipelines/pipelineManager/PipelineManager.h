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
  class MousePicker;
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
                    const std::shared_ptr<MousePicker>& mousePicker,
                    VkDescriptorSetLayout objectDescriptorSetLayout,
                    VkDescriptorSetLayout fontDescriptorSetLayout,
                    VkDescriptorPool descriptorPool,
                    VkCommandPool commandPool,
                    bool shouldDoDots);

    void createNewFrame();

    void renderObject(const std::shared_ptr<RenderObject>& renderObject,
                      PipelineType pipelineType,
                      bool* mousePicked = nullptr);

    void renderLine(glm::vec3 start, glm::vec3 end);

    void renderBendyPlant(const BendyPlant& bendyPlant) const;

    std::shared_ptr<SmokePipeline> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                     uint32_t numParticles = 5'000'000);

    void destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem);

    [[nodiscard]] std::shared_ptr<DotsPipeline> getDotsPipeline();

    [[nodiscard]] std::vector<std::shared_ptr<SmokePipeline>>& getSmokeSystems();

    [[nodiscard]] std::shared_ptr<GuiPipeline> getGuiPipeline();

    void renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                 VkExtent2D extent,
                                 uint32_t currentFrame,
                                 const glm::vec3& viewPosition,
                                 const glm::mat4& viewMatrix,
                                 bool shouldRenderGrid) const;

    [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

    void renderShadowPipeline(const RenderInfo& renderInfo);

    void renderPointLightShadowMapPipeline(const RenderInfo& renderInfo,
                                           const std::shared_ptr<PointLight>& pointLight);

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

    std::shared_ptr<MousePicker> m_mousePicker;

    std::shared_ptr<GuiPipeline> m_guiPipeline;
    std::shared_ptr<DotsPipeline> m_dotsPipeline;

    std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> m_pipelines;
    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;

    std::vector<std::shared_ptr<SmokePipeline>> m_smokeSystems;

    std::unique_ptr<LinePipeline> m_linePipeline;
    std::vector<LineVertex> m_lineVerticesToRender;

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

    void renderRenderObjects(const RenderInfo& renderInfo) const;

    void renderSmokeSystems(const RenderInfo& renderInfo) const;
  };

} // namespace vke

#endif //VKE_PIPELINEMANAGER_H
