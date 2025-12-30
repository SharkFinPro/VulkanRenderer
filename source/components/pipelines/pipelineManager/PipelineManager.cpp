#include "PipelineManager.h"
#include "../implementations/BendyPipeline.h"
#include "../implementations/DotsPipeline.h"
#include "../implementations/GuiPipeline.h"
#include "../implementations/2D/FontPipeline.h"
#include "../implementations/2D/RectPipeline.h"
#include "../implementations/common/PipelineTypes.h"
#include "../implementations/renderObject/BumpyCurtain.h"
#include "../implementations/renderObject/CrossesPipeline.h"
#include "../implementations/renderObject/CubeMapPipeline.h"
#include "../implementations/renderObject/CurtainPipeline.h"
#include "../implementations/renderObject/EllipticalDots.h"
#include "../implementations/renderObject/MagnifyWhirlMosaicPipeline.h"
#include "../implementations/renderObject/NoisyEllipticalDots.h"
#include "../implementations/renderObject/ObjectHighlightPipeline.h"
#include "../implementations/renderObject/ObjectsPipeline.h"
#include "../implementations/renderObject/SnakePipeline.h"
#include "../implementations/renderObject/TexturedPlane.h"
#include "../../assets/AssetManager.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/Renderer.h"
#include <ranges>

namespace vke {

  PipelineManager::PipelineManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                   const std::shared_ptr<Renderer>& renderer,
                                   const std::shared_ptr<LightingManager>& lightingManager,
                                   const std::shared_ptr<AssetManager>& assetManager,
                                   VkDescriptorPool descriptorPool,
                                   VkCommandPool commandPool,
                                   const bool shouldDoDots)
    : m_commandPool(commandPool), m_shouldDoDots(shouldDoDots)
  {
    createPipelines(assetManager, renderer, logicalDevice, lightingManager, descriptorPool);
  }

  std::shared_ptr<DotsPipeline> PipelineManager::getDotsPipeline()
  {
    return m_dotsPipeline;
  }

  std::shared_ptr<GuiPipeline> PipelineManager::getGuiPipeline()
  {
    return m_guiPipeline;
  }

  void PipelineManager::renderShadowPipeline(const RenderInfo& renderInfo,
                                             const std::vector<std::shared_ptr<RenderObject>>* objects) const
  {
    m_shadowPipeline->render(&renderInfo, objects);
  }

  void PipelineManager::renderPointLightShadowMapPipeline(const RenderInfo& renderInfo,
                                                          const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                          const std::shared_ptr<PointLight>& pointLight) const
  {
    m_pointLightShadowMapPipeline->render(&renderInfo, objects, pointLight);
  }

  void PipelineManager::renderBendyPlantPipeline(const RenderInfo& renderInfo,
                                                 const std::vector<BendyPlant>* plants) const
  {
    m_bendyPipeline->render(&renderInfo, plants);
  }

  void PipelineManager::renderGridPipeline(const RenderInfo* renderInfo) const
  {
    m_gridPipeline->render(renderInfo);
  }

  void PipelineManager::renderRenderObjectPipeline(const RenderInfo* renderInfo,
                                                   const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                   const PipelineType pipelineType) const
  {
    const auto it = m_pipelines.find(pipelineType);
    if (it == m_pipelines.end())
    {
      throw std::runtime_error("Pipeline for object type does not exist");
    }

    auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(it->second.get());
    if (!graphicsPipeline)
    {
      throw std::runtime_error("Pipeline for object type is not a GraphicsPipeline");
    }

    graphicsPipeline->displayGui();
    graphicsPipeline->render(renderInfo, objects);
  }

  void PipelineManager::renderSmokePipeline(const RenderInfo* renderInfo,
                                            const std::vector<std::shared_ptr<SmokeSystem>>* systems) const
  {
    m_smokePipeline->render(renderInfo, systems);
  }

  void PipelineManager::computeSmokePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const uint32_t currentFrame,
                                             const std::vector<std::shared_ptr<SmokeSystem>>* systems) const
  {
    m_smokePipeline->compute(commandBuffer, currentFrame, systems);
  }

  void PipelineManager::renderLinePipeline(const RenderInfo* renderInfo,
                                           const std::vector<LineVertex>* lineVertices) const
  {
    m_linePipeline->render(renderInfo, m_commandPool, lineVertices);
  }

  void PipelineManager::renderRectPipeline(const RenderInfo* renderInfo,
                                           const std::vector<Rect>* rects) const
  {
    m_rectPipeline->render(renderInfo, rects);
  }

  void PipelineManager::renderTrianglePipeline(const RenderInfo* renderInfo,
                                               const std::vector<Triangle>* triangles) const
  {
    m_trianglePipeline->render(renderInfo, triangles);
  }

  void PipelineManager::renderEllipsePipeline(const RenderInfo* renderInfo,
                                              const std::vector<Ellipse>* ellipses) const
  {
    m_ellipsePipeline->render(renderInfo, ellipses);
  }

  void PipelineManager::renderFontPipeline(const RenderInfo* renderInfo,
                                           const std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Glyph>>>* glyphs,
                                           const std::shared_ptr<AssetManager>& assetManager) const
  {
    m_fontPipeline->render(renderInfo, glyphs, assetManager);
  }

  void PipelineManager::createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                        const std::shared_ptr<Renderer>& renderer,
                                        const std::shared_ptr<LogicalDevice>& logicalDevice,
                                        const std::shared_ptr<LightingManager>& lightingManager,
                                        VkDescriptorPool descriptorPool)
  {
    create2DPipelines(assetManager, renderer, logicalDevice);

    createRenderObjectPipelines(assetManager, renderer, logicalDevice, lightingManager, descriptorPool);

    createMiscPipelines(assetManager, renderer, logicalDevice, lightingManager, descriptorPool);
  }

  void PipelineManager::create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                          const std::shared_ptr<Renderer>& renderer,
                                          const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();

    m_rectPipeline = std::make_unique<RectPipeline>(logicalDevice, renderPass);

    m_trianglePipeline = std::make_unique<TrianglePipeline>(logicalDevice, renderPass);

    m_ellipsePipeline = std::make_unique<EllipsePipeline>(logicalDevice, renderPass);

    m_fontPipeline = std::make_unique<FontPipeline>(logicalDevice, renderPass, assetManager->getFontDescriptorSetLayout());
  }

  void PipelineManager::createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                                    const std::shared_ptr<Renderer>& renderer,
                                                    const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                    const std::shared_ptr<LightingManager>& lightingManager,
                                                    VkDescriptorPool descriptorPool)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();
    const auto objectDescriptorSetLayout = assetManager->getObjectDescriptorSetLayout();

    m_pipelines[PipelineType::object] = std::make_unique<ObjectsPipeline>(
      logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::objectHighlight] = std::make_unique<ObjectHighlightPipeline>(
      logicalDevice, renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::ellipticalDots] = std::make_unique<EllipticalDots>(
      logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::noisyEllipticalDots] = std::make_unique<NoisyEllipticalDots>(
      logicalDevice, renderPass, m_commandPool, descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::bumpyCurtain] = std::make_unique<BumpyCurtain>(
      logicalDevice, renderPass, m_commandPool, descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::curtain] = std::make_unique<CurtainPipeline>(
      logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::cubeMap] = std::make_unique<CubeMapPipeline>(
      logicalDevice, renderPass, m_commandPool, descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::texturedPlane] = std::make_unique<TexturedPlane>(
      logicalDevice, renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::magnifyWhirlMosaic] = std::make_unique<MagnifyWhirlMosaicPipeline>(
      logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::snake] = std::make_unique<SnakePipeline>(
      logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::crosses] = std::make_unique<CrossesPipeline>(
      logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_shadowPipeline = std::make_unique<ShadowPipeline>(logicalDevice, renderer->getShadowRenderPass(), objectDescriptorSetLayout);

    m_pointLightShadowMapPipeline = std::make_unique<PointLightShadowMapPipeline>(
      logicalDevice, renderer->getShadowCubeRenderPass(), objectDescriptorSetLayout, lightingManager->getPointLightDescriptorSetLayout());
  }

  void PipelineManager::createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                            const std::shared_ptr<Renderer>& renderer,
                                            const std::shared_ptr<LogicalDevice>& logicalDevice,
                                            const std::shared_ptr<LightingManager>& lightingManager,
                                            VkDescriptorPool descriptorPool)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();

    m_guiPipeline = std::make_shared<GuiPipeline>(logicalDevice, renderPass);

    if (m_shouldDoDots)
    {
      m_dotsPipeline = std::make_shared<DotsPipeline>(logicalDevice, m_commandPool, renderPass,
                                                      descriptorPool);
    }

    m_linePipeline = std::make_unique<LinePipeline>(logicalDevice, renderPass);

    m_bendyPipeline = std::make_unique<BendyPipeline>(logicalDevice, renderPass, m_commandPool, descriptorPool,
                                                      lightingManager->getLightingDescriptorSet());

    m_gridPipeline = std::make_unique<GridPipeline>(logicalDevice, renderPass, descriptorPool);

    m_smokePipeline = std::make_unique<SmokePipeline>(logicalDevice, renderPass,
      lightingManager->getLightingDescriptorSet(), assetManager->getSmokeSystemDescriptorSetLayout());
  }
} // namespace vke
