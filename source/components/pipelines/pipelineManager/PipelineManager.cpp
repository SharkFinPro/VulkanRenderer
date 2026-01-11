#include "PipelineManager.h"
#include "../implementations/PipelineConfig.h"
#include "../implementations/PipelineConfig2D.h"
#include "../implementations/common/PipelineTypes.h"
#include "../implementations/renderObject/BumpyCurtain.h"
#include "../implementations/renderObject/CrossesPipeline.h"
#include "../implementations/renderObject/CubeMapPipeline.h"
#include "../implementations/renderObject/CurtainPipeline.h"
#include "../implementations/renderObject/EllipticalDots.h"
#include "../implementations/renderObject/NoisyEllipticalDots.h"
#include "../implementations/renderObject/ObjectsPipeline.h"
#include "../implementations/renderObject/PipelineConfigRenderObject.h"
#include "../implementations/renderObject/SnakePipeline.h"
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

  void PipelineManager::bindGuiPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_guiPipeline->bind(commandBuffer);
  }

  void PipelineManager::bindShadowPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_shadowPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushShadowPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                    const VkShaderStageFlags stageFlags,
                                                    const uint32_t offset,
                                                    const uint32_t size,
                                                    const void* values) const
  {
    m_shadowPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindShadowPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                        VkDescriptorSet descriptorSet,
                                                        const uint32_t location) const
  {
    m_shadowPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
  }

  void PipelineManager::bindPointLightShadowMapPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_pointLightShadowMapPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushPointLightShadowMapPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                                 const VkShaderStageFlags stageFlags,
                                                                 const uint32_t offset,
                                                                 const uint32_t size,
                                                                 const void* values) const
  {
    m_pointLightShadowMapPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindPointLightShadowMapPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                                     VkDescriptorSet descriptorSet,
                                                                     const uint32_t location) const
  {
    m_pointLightShadowMapPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
  }

  void PipelineManager::renderBendyPlantPipeline(const RenderInfo* renderInfo,
                                                 const std::vector<BendyPlant>* plants) const
  {
    m_bendyPipeline->render(renderInfo, plants);
  }

  void PipelineManager::bindGridPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_gridPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushGridPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const VkShaderStageFlags stageFlags,
                                                  const uint32_t offset,
                                                  const uint32_t size,
                                                  const void* values) const
  {
    m_gridPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::renderRenderObjectPipeline(const RenderInfo* renderInfo,
                                                   const std::vector<std::shared_ptr<RenderObject>>* objects,
                                                   const PipelineType pipelineType) const
  {
    const auto graphicsPipeline = getRenderObjectPipeline(pipelineType);

    graphicsPipeline->displayGui();
    graphicsPipeline->render(renderInfo, objects);
  }

  void PipelineManager::bindRenderObjectPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                 const PipelineType pipelineType) const
  {
    const auto graphicsPipeline = getRenderObjectPipeline(pipelineType);

    graphicsPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushRenderObjectPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const PipelineType pipelineType,
                                                          const VkShaderStageFlags stageFlags,
                                                          const uint32_t offset,
                                                          const uint32_t size,
                                                          const void* values) const
  {
    const auto graphicsPipeline = getRenderObjectPipeline(pipelineType);

    graphicsPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindRenderObjectPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                              const PipelineType pipelineType,
                                                              VkDescriptorSet descriptorSet,
                                                              const uint32_t location) const
  {
    const auto graphicsPipeline = getRenderObjectPipeline(pipelineType);

    graphicsPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
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

  void PipelineManager::bindMousePickingPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_mousePickingPipeline->bind(commandBuffer);
  }

  void PipelineManager::pushMousePickingPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const VkShaderStageFlags stageFlags,
                                                          const uint32_t offset,
                                                          const uint32_t size,
                                                          const void* values) const
  {
    m_mousePickingPipeline->pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindMousePickingPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                              VkDescriptorSet descriptorSet,
                                                              const uint32_t location) const
  {
    m_mousePickingPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
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
                                                      VkDescriptorSet descriptorSet,
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

    m_renderObjectPipelines[PipelineType::object] = std::make_shared<ObjectsPipeline>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::objectHighlight] = std::make_shared<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createObjectHighlightPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    m_renderObjectPipelines[PipelineType::ellipticalDots] = std::make_shared<EllipticalDots>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::noisyEllipticalDots] = std::make_shared<NoisyEllipticalDots>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::bumpyCurtain] = std::make_shared<BumpyCurtain>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::curtain] = std::make_shared<CurtainPipeline>(
      m_logicalDevice, renderPass, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::cubeMap] = std::make_shared<CubeMapPipeline>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, objectDescriptorSetLayout);

    m_renderObjectPipelines[PipelineType::texturedPlane] = std::make_shared<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createTexturedPlanePipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    m_renderObjectPipelines[PipelineType::magnifyWhirlMosaic] = std::make_shared<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createMagnifyWhirlMosaicPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    m_renderObjectPipelines[PipelineType::snake] = std::make_shared<SnakePipeline>(
      m_logicalDevice, renderPass, objectDescriptorSetLayout, lightingManager->getLightingDescriptorSet());

    m_renderObjectPipelines[PipelineType::crosses] = std::make_shared<CrossesPipeline>(
      m_logicalDevice, renderPass, m_descriptorPool, objectDescriptorSetLayout,
      lightingManager->getLightingDescriptorSet());

    m_shadowPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createShadowMapPipelineOptions(renderer->getShadowRenderPass(), objectDescriptorSetLayout));

    m_pointLightShadowMapPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createPointLightShadowMapPipelineOptions(renderer->getShadowCubeRenderPass(), objectDescriptorSetLayout,
      lightingManager->getPointLightDescriptorSetLayout()));

    m_mousePickingPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createMousePickingPipelineOptions(renderPass, objectDescriptorSetLayout));
  }

  void PipelineManager::createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                            const std::shared_ptr<Renderer>& renderer,
                                            const std::shared_ptr<LightingManager>& lightingManager)
  {
    const auto renderPass = renderer->getSwapchainRenderPass();

    m_guiPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createUIPipelineOptions(m_logicalDevice, renderPass));

    m_dotsPipeline = std::make_unique<DotsPipeline>(m_logicalDevice, m_commandPool, renderPass, m_descriptorPool);

    m_linePipeline = std::make_unique<LinePipeline>(m_logicalDevice, renderPass);

    m_bendyPipeline = std::make_unique<BendyPipeline>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, lightingManager->getLightingDescriptorSet());

    m_gridPipeline = std::make_unique<GraphicsPipeline>(
      m_logicalDevice, PipelineConfig::createGridPipelineOptions(m_logicalDevice, renderPass));

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

  std::shared_ptr<GraphicsPipeline> PipelineManager::getRenderObjectPipeline(const PipelineType pipelineType) const
  {
    const auto it = m_renderObjectPipelines.find(pipelineType);
    if (it == m_renderObjectPipelines.end())
    {
      throw std::runtime_error("Pipeline for object type does not exist");
    }

    return it->second;
  }
} // namespace vke
