#include "PipelineManager.h"

#include "../implementations/common/PipelineTypes.h"

#include "../implementations/2D/FontPipeline.h"
#include "../implementations/2D/RectPipeline.h"

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

#include "../implementations/BendyPipeline.h"
#include "../implementations/DotsPipeline.h"
#include "../implementations/GuiPipeline.h"
#include "../implementations/SmokePipeline.h"

#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/Renderer.h"

#include <ranges>

namespace vke {

  PipelineManager::PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                   const std::shared_ptr<Renderer>& renderer,
                                   const std::shared_ptr<LightingManager>& lightingManager,
                                   VkDescriptorSetLayout objectDescriptorSetLayout,
                                   VkDescriptorSetLayout fontDescriptorSetLayout,
                                   VkDescriptorPool descriptorPool,
                                   VkCommandPool commandPool,
                                   const bool shouldDoDots)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool), m_descriptorPool(descriptorPool),
      m_renderPass(renderer->getSwapchainRenderPass()), m_lightingManager(lightingManager),
      m_shouldDoDots(shouldDoDots)
  {
    createPipelines(objectDescriptorSetLayout, fontDescriptorSetLayout, renderer);
  }

  std::shared_ptr<SmokePipeline> PipelineManager::createSmokeSystem(glm::vec3 position,
                                                                    uint32_t numParticles)
  {
    auto system = std::make_shared<SmokePipeline>(m_logicalDevice, m_commandPool, m_renderPass, m_descriptorPool,
                                                  position, numParticles, m_lightingManager->getLightingDescriptorSet());

    m_smokeSystems.push_back(system);

    return system;
  }

  void PipelineManager::destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem)
  {
    const auto system = std::ranges::find(m_smokeSystems, smokeSystem);

    if (system == m_smokeSystems.end())
    {
      return;
    }

    m_logicalDevice->waitIdle();
    m_smokeSystems.erase(system);
  }

  std::shared_ptr<DotsPipeline> PipelineManager::getDotsPipeline()
  {
    return m_dotsPipeline;
  }

  std::vector<std::shared_ptr<SmokePipeline>>& PipelineManager::getSmokeSystems()
  {
    return m_smokeSystems;
  }

  std::shared_ptr<GuiPipeline> PipelineManager::getGuiPipeline()
  {
    return m_guiPipeline;
  }

  void PipelineManager::renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                const VkExtent2D extent,
                                                const uint32_t currentFrame,
                                                const glm::vec3& viewPosition,
                                                const glm::mat4& viewMatrix,
                                                const bool shouldRenderGrid) const
  {
    // if (m_shouldDoDots)
    // {
    //   m_dotsPipeline->render(&renderInfo, nullptr);
    // }
    //
    // if (shouldRenderGrid)
    // {
    //   m_gridPipeline->render(&renderInfo);
    // }
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

    if (it->first == PipelineType::objectHighlight)
    {
      return;
    }

    auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(it->second.get());
    if (!graphicsPipeline)
    {
      throw std::runtime_error("Pipeline for object type is not a GraphicsPipeline");
    }

    graphicsPipeline->displayGui();
    graphicsPipeline->render(renderInfo, objects);
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

  void PipelineManager::createPipelines(VkDescriptorSetLayout objectDescriptorSetLayout,
                                        VkDescriptorSetLayout fontDescriptorSetLayout,
                                        const std::shared_ptr<Renderer>& renderer)
  {
    m_pipelines[PipelineType::object] = std::make_unique<ObjectsPipeline>(
      m_logicalDevice, m_renderPass, objectDescriptorSetLayout,
      m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::objectHighlight] = std::make_unique<ObjectHighlightPipeline>(
      m_logicalDevice, m_renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::ellipticalDots] = std::make_unique<EllipticalDots>(
      m_logicalDevice, m_renderPass, objectDescriptorSetLayout, m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::noisyEllipticalDots] = std::make_unique<NoisyEllipticalDots>(
      m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::bumpyCurtain] = std::make_unique<BumpyCurtain>(
      m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::curtain] = std::make_unique<CurtainPipeline>(
      m_logicalDevice, m_renderPass, m_descriptorPool, objectDescriptorSetLayout,
      m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::cubeMap] = std::make_unique<CubeMapPipeline>(
      m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::texturedPlane] = std::make_unique<TexturedPlane>(
      m_logicalDevice, m_renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::magnifyWhirlMosaic] = std::make_unique<MagnifyWhirlMosaicPipeline>(
      m_logicalDevice, m_renderPass, m_descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::snake] = std::make_unique<SnakePipeline>(
      m_logicalDevice, m_renderPass, objectDescriptorSetLayout, m_lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::crosses] = std::make_unique<CrossesPipeline>(
      m_logicalDevice, m_renderPass, m_descriptorPool, objectDescriptorSetLayout,
      m_lightingManager->getLightingDescriptorSet());

    m_guiPipeline = std::make_shared<GuiPipeline>(m_logicalDevice, m_renderPass);

    if (m_shouldDoDots)
    {
      m_dotsPipeline = std::make_shared<DotsPipeline>(m_logicalDevice, m_commandPool, m_renderPass,
                                                      m_descriptorPool);
    }

    m_linePipeline = std::make_unique<LinePipeline>(m_logicalDevice, m_renderPass);

    m_bendyPipeline = std::make_unique<BendyPipeline>(m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool,
                                                      m_lightingManager->getLightingDescriptorSet());

    m_gridPipeline = std::make_unique<GridPipeline>(m_logicalDevice, m_renderPass, m_descriptorPool);

    m_shadowPipeline = std::make_unique<ShadowPipeline>(m_logicalDevice, renderer->getShadowRenderPass(), objectDescriptorSetLayout);

    m_pointLightShadowMapPipeline = std::make_unique<PointLightShadowMapPipeline>(
      m_logicalDevice, renderer->getShadowCubeRenderPass(), objectDescriptorSetLayout, m_lightingManager->getPointLightDescriptorSetLayout());

    m_rectPipeline = std::make_unique<RectPipeline>(m_logicalDevice, m_renderPass);

    m_trianglePipeline = std::make_unique<TrianglePipeline>(m_logicalDevice, m_renderPass);

    m_ellipsePipeline = std::make_unique<EllipsePipeline>(m_logicalDevice, m_renderPass);

    m_fontPipeline = std::make_unique<FontPipeline>(m_logicalDevice, m_renderPass, fontDescriptorSetLayout);
  }

} // namespace vke
