#include "PipelineManager.h"
#include "PipelineConfig.h"
#include "PipelineConfig2D.h"
#include "PipelineConfigRenderObject.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../RayTracingPipeline.h"
#include "../../assets/AssetManager.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../renderingManager/Renderer.h"
#include "../../renderingManager/RenderingManager.h"
#include <ranges>

namespace vke {

  PipelineManager::PipelineManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                   const std::shared_ptr<RenderingManager>& renderingManager,
                                   const std::shared_ptr<LightingManager>& lightingManager,
                                   const std::shared_ptr<AssetManager>& assetManager)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    createPipelines(assetManager, renderingManager, lightingManager);

    RayTracingPipelineConfig rtConfig {
      .shaders {
        .rayGenerationShader = "assets/shaders/rayTracing/object.rgen.spv",
        .missShader = "assets/shaders/rayTracing/object.rmiss.spv",
        .closestHitShader = "assets/shaders/rayTracing/object.rchit.spv"
      },
      .descriptorSetLayouts {
        assetManager->getRayTracingDescriptorSetLayout(),
        lightingManager->getLightingDescriptorSet()->getDescriptorSetLayout()
      }
    };
    m_rayTracingPipeline = std::make_unique<RayTracingPipeline>(m_logicalDevice, rtConfig);
  }

  PipelineManager::~PipelineManager()
  {
    m_logicalDevice->destroyDescriptorPool(m_descriptorPool);

    m_logicalDevice->destroyCommandPool(m_commandPool);
  }

  void PipelineManager::bindGraphicsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const PipelineType pipelineType) const
  {
    const auto& graphicsPipeline = getGraphicsPipeline(pipelineType);

    graphicsPipeline.bind(commandBuffer);
  }

  void PipelineManager::pushGraphicsPipelineConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                      const PipelineType pipelineType,
                                                      const VkShaderStageFlags stageFlags,
                                                      const uint32_t offset,
                                                      const uint32_t size,
                                                      const void* values) const
  {
    const auto& graphicsPipeline = getGraphicsPipeline(pipelineType);

    graphicsPipeline.pushConstants(commandBuffer, stageFlags, offset, size, values);
  }

  void PipelineManager::bindGraphicsPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const PipelineType pipelineType,
                                                          VkDescriptorSet descriptorSet,
                                                          const uint32_t location) const
  {
    const auto& graphicsPipeline = getGraphicsPipeline(pipelineType);

    graphicsPipeline.bindDescriptorSet(commandBuffer, descriptorSet, location);
  }

  void PipelineManager::renderDotsPipeline(const RenderInfo* renderInfo) const
  {
    m_dotsPipeline->render(renderInfo);
  }

  void PipelineManager::computeDotsPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                            const uint32_t currentFrame) const
  {
    m_dotsPipeline->compute(commandBuffer, currentFrame);
  }

  void PipelineManager::renderBendyPlantPipeline(const RenderInfo* renderInfo,
                                                 const std::vector<BendyPlant>* plants) const
  {
    m_bendyPipeline->render(renderInfo, plants);
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

  void PipelineManager::doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     const VkExtent2D extent) const
  {
    m_rayTracingPipeline->doRayTracing(commandBuffer, extent);
  }

  void PipelineManager::bindRayTracingPipelineDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                            VkDescriptorSet descriptorSet,
                                                            const uint32_t location) const
  {
    m_rayTracingPipeline->bindDescriptorSet(commandBuffer, descriptorSet, location);
  }

  void PipelineManager::createGraphicsPipeline(const PipelineType pipelineType,
                                               const GraphicsPipelineOptions& graphicsPipelineOptions)
  {
    m_graphicsPipelines[pipelineType] = std::make_unique<GraphicsPipeline>(m_logicalDevice, graphicsPipelineOptions);
  }

  void PipelineManager::createPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                        const std::shared_ptr<RenderingManager>& renderingManager,
                                        const std::shared_ptr<LightingManager>& lightingManager)
  {
    create2DPipelines(assetManager, renderingManager);

    createRenderObjectPipelines(assetManager, renderingManager, lightingManager);

    createMiscPipelines(assetManager, renderingManager, lightingManager);
  }

  void PipelineManager::create2DPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                          const std::shared_ptr<RenderingManager>& renderingManager)
  {
    const auto renderPass = renderingManager->getRenderer()->getSwapchainRenderPass();

    createGraphicsPipeline(PipelineType::rect,
      PipelineConfig::createRectPipelineOptions(m_logicalDevice, renderPass));

    createGraphicsPipeline(PipelineType::triangle,
      PipelineConfig::createTrianglePipelineOptions(m_logicalDevice, renderPass));

    createGraphicsPipeline(PipelineType::ellipse,
      PipelineConfig::createEllipsePipelineOptions(m_logicalDevice, renderPass));

    createGraphicsPipeline(PipelineType::font,
      PipelineConfig::createFontPipelineOptions(m_logicalDevice, renderPass, assetManager->getFontDescriptorSetLayout()));
  }

  void PipelineManager::createRenderObjectPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                                    const std::shared_ptr<RenderingManager>& renderingManager,
                                                    const std::shared_ptr<LightingManager>& lightingManager)
  {
    const auto renderPass = renderingManager->getRenderer()->getSwapchainRenderPass();
    const auto objectDescriptorSetLayout = assetManager->getObjectDescriptorSetLayout();
    const auto lightingDescriptorSetLayout = lightingManager->getLightingDescriptorSet()->getDescriptorSetLayout();

    createGraphicsPipeline(PipelineType::object,
      PipelineConfig::createObjectsPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::objectHighlight,
      PipelineConfig::createObjectHighlightPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::ellipticalDots,
      PipelineConfig::createEllipticalDotsPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::noisyEllipticalDots,
      PipelineConfig::createNoisyEllipticalDotsPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout, renderingManager->getRenderer3D()->getNoiseDescriptorSetLayout()));

    createGraphicsPipeline(PipelineType::bumpyCurtain,
      PipelineConfig::createBumpyCurtainPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout, renderingManager->getRenderer3D()->getNoiseDescriptorSetLayout()));

    createGraphicsPipeline(PipelineType::curtain,
      PipelineConfig::createCurtainPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::cubeMap,
      PipelineConfig::createCubeMapPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      renderingManager->getRenderer3D()->getCubeMapDescriptorSetLayout()));

    createGraphicsPipeline(PipelineType::texturedPlane,
      PipelineConfig::createTexturedPlanePipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::magnifyWhirlMosaic,
      PipelineConfig::createMagnifyWhirlMosaicPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::snake,
      PipelineConfig::createSnakePipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::crosses,
      PipelineConfig::createCrossesPipelineOptions(m_logicalDevice, renderPass, objectDescriptorSetLayout,
      lightingDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::shadow,
      PipelineConfig::createShadowMapPipelineOptions(renderingManager->getRenderer()->getShadowRenderPass(), objectDescriptorSetLayout));

    createGraphicsPipeline(PipelineType::pointLightShadowMap,
      PipelineConfig::createPointLightShadowMapPipelineOptions(renderingManager->getRenderer()->getShadowCubeRenderPass(), objectDescriptorSetLayout,
      lightingManager->getPointLightDescriptorSetLayout()));

    createGraphicsPipeline(PipelineType::mousePicking,
      PipelineConfig::createMousePickingPipelineOptions(renderPass, objectDescriptorSetLayout));
  }

  void PipelineManager::createMiscPipelines(const std::shared_ptr<AssetManager>& assetManager,
                                            const std::shared_ptr<RenderingManager>& renderingManager,
                                            const std::shared_ptr<LightingManager>& lightingManager)
  {
    const auto renderPass = renderingManager->getRenderer()->getSwapchainRenderPass();

    createGraphicsPipeline(PipelineType::gui,
      PipelineConfig::createUIPipelineOptions(m_logicalDevice, renderPass));

    m_dotsPipeline = std::make_unique<DotsPipeline>(m_logicalDevice, m_commandPool, renderPass, m_descriptorPool);

    m_linePipeline = std::make_unique<LinePipeline>(m_logicalDevice, renderPass);

    m_bendyPipeline = std::make_unique<BendyPipeline>(
      m_logicalDevice, renderPass, m_commandPool, m_descriptorPool, lightingManager->getLightingDescriptorSet());

    createGraphicsPipeline(PipelineType::grid,
      PipelineConfig::createGridPipelineOptions(m_logicalDevice, renderPass));

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
    const std::array<VkDescriptorPoolSize, 3> poolSizes {{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight() * 30},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_logicalDevice->getMaxFramesInFlight() * 2},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_logicalDevice->getMaxFramesInFlight() * 2}
    }};

    const VkDescriptorPoolCreateInfo poolCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * 30,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
  }

  const GraphicsPipeline& PipelineManager::getGraphicsPipeline(const PipelineType pipelineType) const
  {
    const auto it = m_graphicsPipelines.find(pipelineType);
    if (it == m_graphicsPipelines.end())
    {
      throw std::runtime_error("Pipeline for the given type does not exist");
    }

    return *it->second;
  }
} // namespace vke
