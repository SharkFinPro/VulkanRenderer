#ifndef VKE_PIPELINEMANAGER_H
#define VKE_PIPELINEMANAGER_H

#include "../RayTracingPipeline.h"
#include "../implementations/BendyPipeline.h"
#include "../implementations/DotsPipeline.h"
#include "../implementations/LinePipeline.h"
#include "../implementations/SmokePipeline.h"
#include "../implementations/common/PipelineTypes.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class LightingManager;
  class DotsPipeline;
  class PointLight;
  class Pipeline;
  class RenderingManager;
  class RenderObject;

  class PipelineManager {
  public:
    PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                    const std::shared_ptr<RenderingManager>& renderingManager,
                    const std::shared_ptr<LightingManager>& lightingManager,
                    const std::shared_ptr<AssetManager>& assetManager);

    void bindGraphicsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              PipelineType pipelineType) const;

    template<typename T>
    void pushGraphicsPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                       PipelineType pipelineType,
                                       vk::ShaderStageFlags stageFlags,
                                       uint32_t offset,
                                       const T& data) const;

    void bindGraphicsPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           PipelineType pipelineType,
                                           vk::DescriptorSet descriptorSet,
                                           uint32_t location) const;

    void renderDotsPipeline(const RenderInfo* renderInfo) const;

    void computeDotsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                             uint32_t currentFrame) const;

    void renderBendyPlantPipeline(const RenderInfo* renderInfo,
                                  const std::vector<BendyPlant>* plants) const;

    void renderSmokePipeline(const RenderInfo* renderInfo,
                             const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void computeSmokePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                              uint32_t currentFrame,
                              const std::vector<std::shared_ptr<SmokeSystem>>* systems) const;

    void renderLinePipeline(const RenderInfo* renderInfo,
                            const std::vector<LineVertex>* lineVertices) const;

    void doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                      vk::Extent2D extent) const;

    void bindRayTracingPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             vk::DescriptorSet descriptorSet,
                                             uint32_t location) const;

    template<typename T>
    void pushRayTracingPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                         vk::ShaderStageFlags stageFlags,
                                         uint32_t offset,
                                         const T& data) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::CommandPool m_commandPool = nullptr;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    std::unique_ptr<DotsPipeline> m_dotsPipeline;

    std::unique_ptr<SmokePipeline> m_smokePipeline;

    std::unique_ptr<LinePipeline> m_linePipeline;

    std::unique_ptr<BendyPipeline> m_bendyPipeline;

    std::unordered_map<PipelineType, std::unique_ptr<GraphicsPipeline>> m_graphicsPipelines;

    std::unique_ptr<RayTracingPipeline> m_rayTracingPipeline;

    void createGraphicsPipeline(PipelineType pipelineType,
                                const GraphicsPipelineOptions& graphicsPipelineOptions);

    void createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                         const std::shared_ptr<RenderingManager>& renderingManager,
                         const std::shared_ptr<LightingManager>& lightingManager);

    void create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                           const std::shared_ptr<RenderingManager>& renderingManager);

    void createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                     const std::shared_ptr<RenderingManager>& renderingManager,
                                     const std::shared_ptr<LightingManager>& lightingManager);

    void createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                             const std::shared_ptr<RenderingManager>& renderingManager,
                             const std::shared_ptr<LightingManager>& lightingManager);

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] const GraphicsPipeline& getGraphicsPipeline(PipelineType pipelineType) const;

    void createRayTracingPipeline(const std::shared_ptr<AssetManager>& assetManager,
                                  const std::shared_ptr<LightingManager>& lightingManager);
  };

  template<typename T>
  void PipelineManager::pushGraphicsPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                      const PipelineType pipelineType,
                                                      const vk::ShaderStageFlags stageFlags,
                                                      const uint32_t offset,
                                                      const T& data) const
  {
    const auto& graphicsPipeline = getGraphicsPipeline(pipelineType);

    graphicsPipeline.pushConstants<T>(commandBuffer, stageFlags, offset, data);
  }

  template<typename T>
  void PipelineManager::pushRayTracingPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                        vk::ShaderStageFlags stageFlags,
                                                        uint32_t offset,
                                                        const T& data) const
  {
    m_rayTracingPipeline->pushConstants<T>(commandBuffer, stageFlags, offset, data);
  }
} // namespace vke

#endif //VKE_PIPELINEMANAGER_H