#include "PipelineManager.h"
#include "../implementations/PipelineConfig2D.h"
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
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../renderingManager/Renderer.h"
#include <ranges>

namespace vke {

  PipelineManager::PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                   const std::shared_ptr<Renderer>& renderer,
                                   const std::shared_ptr<LightingManager>& lightingManager,
                                   const std::shared_ptr<AssetManager>& assetManager)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    createPipelines(assetManager, renderer, lightingManager);
  }

  PipelineManager::~PipelineManager()
  {
    m_logicalDevice->destroyDescriptorPool(m_descriptorPool);

    m_logicalDevice->destroyCommandPool(m_commandPool);
  }

  void PipelineManager::renderDotsPipeline(const RenderInfo* renderInfo) const
  {
    m_dotsPipeline->render(renderInfo, nullptr);
  }

  void PipelineManager::computeDotsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                            const uint32_t currentFrame) const
  {
    m_dotsPipeline->compute(commandBuffer, currentFrame);
  }

  void PipelineManager::renderGuiPipeline(const RenderInfo* renderInfo) const
  {
    m_guiPipeline->render(renderInfo);
  }

  void PipelineManager::renderShadowPipeline(const RenderInfo* renderInfo,
                                             const std::vector<std::shared_ptr<RenderObject>>* objects) const
  {
    m_shadowPipeline->render(renderInfo, objects);
  }

  void PipelineManager::renderPointLightShadowMapPipeline(const RenderInfo* renderInfo,
                                                          const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                          const std::shared_ptr<PointLight>& pointLight) const
  {
    m_pointLightShadowMapPipeline->render(renderInfo, objects, pointLight);
  }

  void PipelineManager::renderBendyPlantPipeline(const RenderInfo* renderInfo,
                                                 const std::vector<BendyPlant>* plants) const
  {
    m_bendyPipeline->render(renderInfo, plants);
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

  void PipelineManager::renderMousePickingPipeline(const RenderInfo* renderInfo,
                                                   const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects) const
  {
    m_mousePickingPipeline->render(renderInfo, objects);
  }

  void PipelineManager::renderLinePipeline(const RenderInfo* renderInfo,
                                           const std::vector<LineVertex>* lineVertices) const
  {
    m_linePipeline->render(renderInfo, m_commandPool, lineVertices);
  }

  void PipelineManager::bindRectPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_rectPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushRectPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const VkShaderStageFlags stageFlags,
                                                  const uint32_t offset,
                                                  const uint32_t size,
                                                  const void* values) const
  {
    m_rectPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindTrianglePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_trianglePipeline->bind(commandBuffer);
  }

  void PipelineManager::pushTrianglePipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                      const VkShaderStageFlags stageFlags,
                                                      const uint32_t offset,
                                                      const uint32_t size,
                                                      const void* values) const
  {
    m_trianglePipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindEllipsePipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_ellipsePipeline->bind(commandBuffer);
  }

  void PipelineManager::pushEllipsePipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                     const VkShaderStageFlags stageFlags,
                                                     const uint32_t offset,
                                                     const uint32_t size,
                                                     const void* values) const
  {
    m_ellipsePipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindFontPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_fontPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushFontPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const VkShaderStageFlags stageFlags,
                                                  const uint32_t offset,
                                                  const uint32_t size,
                                                  const void* values) const
  {
    m_fontPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindFontPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                      const VkDescriptorSet descriptorSet,
                                                      const uint32_t location) const
  {
    m_fontPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
  }

  void PipelineManager::createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                        const std::shared_ptr<Renderer>& renderer,
                                        const std::shared_ptr<LightingManager>& lightingManager)
  {
    create2DPipelines(assetManager, renderer);

    createRenderObjectPipelines(assetManager, renderer, lightingManager);

    createMiscPipelines(assetManager, renderer, lightingManager);
  }

  void PipelineManager::create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                          const std::shared_ptr<Renderer>& renderer)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();

    m_rectPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createRectPipelineOptions(m_logicalDevice, renderPass));

    m_trianglePipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createTrianglePipelineOptions(m_logicalDevice, renderPass));

    m_ellipsePipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createEllipsePipelineOptions(m_logicalDevice, renderPass));

    m_fontPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createFontPipelineOptions(m_logicalDevice, renderPass, assetManager->getFontDescriptorSetLayout()));
  }

  void PipelineManager::createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                                    const std::shared_ptr<Renderer>& renderer,
                                                    const std::shared_ptr<LightingManager>& lightingManager)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();
    const auto objectDescriptorSetLayout = assetManager->getObjectDescriptorSetLayout();

    m_pipelines[PipelineType::object] = std::make_unique<ObjectsPipeline>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::objectHighlight] = std::make_unique<ObjectHighlightPipeline>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::ellipticalDots] = std::make_unique<EllipticalDots>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::noisyEllipticalDots] = std::make_unique<NoisyEllipticalDots>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::bumpyCurtain] = std::make_unique<BumpyCurtain>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::curtain] = std::make_unique<CurtainPipeline>(
      m_logicalDevice, renderPass, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::cubeMap] = std::make_unique<CubeMapPipeline>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::texturedPlane] = std::make_unique<TexturedPlane>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout);

    m_pipelines[PipelineType::magnifyWhirlMosaic] = std::make_unique<MagnifyWhirlMosaicPipeline>(
      m_logicalDevice, renderPass, m_descriptorPool, objectDescriptorSetLayout);

    m_pipelines[PipelineType::snake] = std::make_unique<SnakePipeline>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_pipelines[PipelineType::crosses] = std::make_unique<CrossesPipeline>(
      m_logicalDevice, renderPass, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_shadowPipeline = std::make_unique<ShadowPipeline>(
      m_logicalDevice, renderer->getShadowRenderPass(), objectDescriptorSetLayout);

    m_pointLightShadowMapPipeline = std::make_unique<PointLightShadowMapPipeline>(
      m_logicalDevice, renderer->getShadowCubeRenderPass(), objectDescriptorSetLayout,
      lightingManager->getPointLightDescriptorSetLayout());

    m_mousePickingPipeline = std::make_unique<MousePickingPipeline>(
      m_logicalDevice, renderer->getMousePickingRenderPass(), objectDescriptorSetLayout);
  }

  void PipelineManager::createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                            const std::shared_ptr<Renderer>& renderer,
                                            const std::shared_ptr<LightingManager>& lightingManager)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();

    m_guiPipeline = std::make_unique<GuiPipeline>(m_logicalDevice, renderPass);

    m_dotsPipeline = std::make_unique<DotsPipeline>(m_logicalDevice, m_commandPool, renderPass, m_descriptorPool);

    m_linePipeline = std::make_unique<LinePipeline>(m_logicalDevice, renderPass);

    m_bendyPipeline = std::make_unique<BendyPipeline>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, lightingManager->getLightingDescriptorSet());

    m_gridPipeline = std::make_unique<GridPipeline>(m_logicalDevice, renderPass, m_descriptorPool);

    m_smokePipeline = std::make_unique<SmokePipeline>(
      m_logicalDevice, renderPass, lightingManager->getLightingDescriptorSet(),
      assetManager->getSmokeSystemDescriptorSetLayout());
  }

  void PipelineManager::createCommandPool()
  {
    const VkCommandPoolCreateInfo poolInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void PipelineManager::createDescriptorPool()
  {
    const std::array<VkDescriptorPoolSize, 1> poolSizes {{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight() * 30}
    }};

    const VkDescriptorPoolCreateInfo poolCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * 30,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
  }
} // namespace vke
