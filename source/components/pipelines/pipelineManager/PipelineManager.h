#ifndef VKE_PIPELINEMANAGER_H
#define VKE_PIPELINEMANAGER_H

#include "../implementations/BendyPipeline.h"
#include "../implementations/DotsPipeline.h"
#include "../implementations/LinePipeline.h"
#include "../implementations/SmokePipeline.h"
#include "../implementations/renderObject/MousePickingPipeline.h"
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
    PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                    const std::shared_ptr<Renderer>& renderer,
                    const std::shared_ptr<LightingManager>& lightingManager,
                    const std::shared_ptr<AssetManager>& assetManager);

    ~PipelineManager();

    void renderDotsPipeline(const RenderInfo* renderInfo) const;

    void computeDotsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                             uint32_t currentFrame) const;

    void bindGuiPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void renderShadowPipeline(const RenderInfo* renderInfo,
                              const std::vector<std::shared_ptr<RenderObject>>* objects) const;

    void renderPointLightShadowMapPipeline(const RenderInfo* renderInfo,
                                           const std::vector<std::shared_ptr<RenderObject>>* objects,
                                           const std::shared_ptr<PointLight>& pointLight) const;

    void renderBendyPlantPipeline(const RenderInfo* renderInfo,
                                  const std::vector<BendyPlant>* plants) const;

    void bindGridPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void pushGridPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   VkShaderStageFlags stageFlags,
                                   uint32_t offset,
                                   uint32_t size,
                                   const void* values) const;

    void renderRenderObjectPipeline(const RenderInfo* renderInfo,
                                    const std::vector<std::shared_ptr<RenderObject>>* objects,
                                    PipelineType pipelineType) const;

    void bindRenderObjectPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                  PipelineType pipelineType) const;

    void bindRenderObjectPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                               PipelineType pipelineType,
                                               VkDescriptorSet descriptorSet,
                                               uint32_t location) const;

    void renderSmokePipeline(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void computeSmokePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              uint32_t currentFrame,
                              const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void renderMousePickingPipeline(const RenderInfo* renderInfo,
                                    const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects) const;

    void renderLinePipeline(const RenderInfo* renderInfo,
                            const std::vector<LineVertex>* lineVertices) const;

    void bindRectPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void pushRectPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   VkShaderStageFlags stageFlags,
                                   uint32_t offset,
                                   uint32_t size,
                                   const void* values) const;

    void bindTrianglePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void pushTrianglePipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                       VkShaderStageFlags stageFlags,
                                       uint32_t offset,
                                       uint32_t size,
                                       const void* values) const;

    void bindEllipsePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void pushEllipsePipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      VkShaderStageFlags stageFlags,
                                      uint32_t offset,
                                      uint32_t size,
                                      const void* values) const;

    void bindFontPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void pushFontPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   VkShaderStageFlags stageFlags,
                                   uint32_t offset,
                                   uint32_t size,
                                   const void* values) const;

    void bindFontPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                       VkDescriptorSet descriptorSet,
                                       uint32_t location) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::unique_ptr<GraphicsPipeline> m_guiPipeline;

    std::unique_ptr<DotsPipeline> m_dotsPipeline;

    std::unordered_map<PipelineType, std::shared_ptr<Pipeline>> m_renderObjectPipelines;

    std::unique_ptr<SmokePipeline> m_smokePipeline;

    std::unique_ptr<LinePipeline> m_linePipeline;

    std::unique_ptr<BendyPipeline> m_bendyPipeline;

    std::unique_ptr<GraphicsPipeline> m_gridPipeline;

    std::unique_ptr<PointLightShadowMapPipeline> m_pointLightShadowMapPipeline;

    std::unique_ptr<ShadowPipeline> m_shadowPipeline;

    std::unique_ptr<MousePickingPipeline> m_mousePickingPipeline;

    std::unique_ptr<GraphicsPipeline> m_rectPipeline;

    std::unique_ptr<GraphicsPipeline> m_trianglePipeline;

    std::unique_ptr<GraphicsPipeline> m_ellipsePipeline;

    std::unique_ptr<GraphicsPipeline> m_fontPipeline;

    void createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                         const std::shared_ptr<Renderer>& renderer,
                         const std::shared_ptr<LightingManager>& lightingManager);

    void create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                           const std::shared_ptr<Renderer>& renderer);

    void createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                     const std::shared_ptr<Renderer>& renderer,
                                     const std::shared_ptr<LightingManager>& lightingManager);

    void createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                             const std::shared_ptr<Renderer>& renderer,
                             const std::shared_ptr<LightingManager>& lightingManager);

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] std::shared_ptr<GraphicsPipeline> getRenderObjectPipeline(PipelineType pipelineType) const;
  };

} // namespace vke

#endif //VKE_PIPELINEMANAGER_H
