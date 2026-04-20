#include "Renderer3D.h"
#include "MousePicker.h"
#include "../../assets/AssetManager.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../assets/textures/Texture3D.h"
#include "../../assets/textures/TextureCubemap.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"

namespace vke {

  Renderer3D::Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<AssetManager> assetManager,
                         std::shared_ptr<Window> window)
    : m_logicalDevice(std::move(logicalDevice)), m_assetManager(std::move(assetManager))
  {
    createCommandPool();

    createDescriptorPool();

    m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice, std::move(window), m_commandPool);

    createDescriptorSets();

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      m_rayTracer = std::make_unique<RayTracer>(m_logicalDevice, m_assetManager, m_commandPool, m_descriptorPool);
    }
  }

  void Renderer3D::updateLightingManager(const std::shared_ptr<LightingManager>& lightingManager,
                                         const uint32_t currentFrame) const
  {
    lightingManager->update(currentFrame, m_viewPosition);
  }

  void Renderer3D::renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const uint32_t currentFrame) const
  {
    lightingManager->renderShadowMaps(commandBuffer, pipelineManager, &m_renderObjectsToRenderFlattened, currentFrame);
  }

  void Renderer3D::renderMousePicking(const RenderInfo* renderInfo,
                                      const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    const RenderInfo renderInfoMousePicking {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent
    };

    m_mousePicker->render(&renderInfoMousePicking, pipelineManager);
  }

  void Renderer3D::handleRenderedMousePickingImage(const vk::Image image) const
  {
    m_mousePicker->handleRenderedMousePickingImage(image);
  }

  void Renderer3D::render(const RenderInfo* renderInfo,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          const std::shared_ptr<LightingManager>& lightingManager)
  {
    displayGui();

    const RenderInfo renderInfo3D {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent
    };

    auto& cubeMapPC = std::get<CubeMapPushConstant>(m_pushConstants.at(PipelineType::cubeMap).data);
    cubeMapPC.position = renderInfo3D.viewPosition;

    renderRenderObjectsByPipeline(&renderInfo3D, pipelineManager, lightingManager);

    pipelineManager->renderBendyPlantPipeline(&renderInfo3D, &m_bendyPlantsToRender);

    renderSmokeSystems(&renderInfo3D, pipelineManager);

    pipelineManager->renderLinePipeline(&renderInfo3D, &m_lineVerticesToRender);

    if (m_shouldRenderGrid)
    {
      renderGrid(pipelineManager, &renderInfo3D);
    }
  }

  void Renderer3D::doRayTracing(const RenderInfo* renderInfo,
                                const std::shared_ptr<PipelineManager>& pipelineManager,
                                const std::shared_ptr<LightingManager>& lightingManager,
                                const ImageResource& imageResource) const
  {
    m_rayTracer->doRayTracing(
      renderInfo,
      pipelineManager,
      lightingManager,
      imageResource,
      m_renderObjectsToRenderFlattened,
      m_cloudToRender,
      m_smokeVolumesToRender,
      m_viewPosition,
      m_viewMatrix
    );
  }

  void Renderer3D::createNewFrame()
  {
    for (auto& [_, objects] : m_renderObjectsToRender)
    {
      objects.clear();
    }

    m_renderObjectsToRenderFlattened.clear();

    m_lineVerticesToRender.clear();

    m_bendyPlantsToRender.clear();

    m_mousePicker->clearObjectsToMousePick();

    m_smokeSystemsToRender.clear();

    m_smokeVolumesToRender.clear();

    m_cloudToRender.reset();
  }

  void Renderer3D::enableGrid()
  {
    m_shouldRenderGrid = true;
  }

  void Renderer3D::disableGrid()
  {
    m_shouldRenderGrid = false;
  }

  bool Renderer3D::isGridEnabled() const
  {
    return m_shouldRenderGrid;
  }

  void Renderer3D::setCameraParameters(const glm::vec3 position,
                                       const glm::mat4& viewMatrix)
  {
    m_viewPosition = position;
    m_viewMatrix = viewMatrix;
  }

  std::shared_ptr<MousePicker> Renderer3D::getMousePicker() const
  {
    return m_mousePicker;
  }

  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& Renderer3D::getRenderObjectsToRender()
  {
    return m_renderObjectsToRender;
  }

  void Renderer3D::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                const PipelineType pipelineType,
                                bool* mousePicked)
  {
    m_renderObjectsToRender[pipelineType].push_back(renderObject);

    m_renderObjectsToRenderFlattened.push_back(renderObject);

    if (mousePicked)
    {
      m_mousePicker->renderObject(renderObject, mousePicked);
    }
  }

  void Renderer3D::renderLine(const glm::vec3 start,
                              const glm::vec3 end)
  {
    m_lineVerticesToRender.insert(m_lineVerticesToRender.end(), { LineVertex{start}, LineVertex{end} });
  }

  void Renderer3D::renderBendyPlant(const BendyPlant& bendyPlant)
  {
    m_bendyPlantsToRender.push_back(bendyPlant);
  }

  void Renderer3D::renderSmokeSystem(const std::shared_ptr<SmokeSystem>& smokeSystem)
  {
    m_smokeSystemsToRender.push_back(smokeSystem);
  }

  const std::vector<std::shared_ptr<SmokeSystem>>& Renderer3D::getSmokeSystems() const
  {
    return m_smokeSystemsToRender;
  }

  vk::DescriptorSetLayout Renderer3D::getNoiseDescriptorSetLayout() const
  {
    return m_noiseDescriptorSet->getDescriptorSetLayout();
  }

  vk::DescriptorSetLayout Renderer3D::getCubeMapDescriptorSetLayout() const
  {
    return m_cubeMapDescriptorSet->getDescriptorSetLayout();
  }

  void Renderer3D::setCloudToRender(std::shared_ptr<Cloud> cloud)
  {
    m_cloudToRender = std::move(cloud);
  }

  void Renderer3D::renderSmokeVolume(std::shared_ptr<SmokeVolume> smokeVolume)
  {
    m_smokeVolumesToRender.push_back(std::move(smokeVolume));
  }

  void Renderer3D::createCommandPool()
  {
    const vk::CommandPoolCreateInfo poolInfo {
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void Renderer3D::createDescriptorPool()
  {
    std::vector<vk::DescriptorPoolSize> poolSizes {{
      {vk::DescriptorType::eCombinedImageSampler, m_logicalDevice->getMaxFramesInFlight() * 256},
      {vk::DescriptorType::eStorageImage, m_logicalDevice->getMaxFramesInFlight() * 4},
      {vk::DescriptorType::eStorageBuffer, m_logicalDevice->getMaxFramesInFlight() * 10},
    }};

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      poolSizes.push_back({vk::DescriptorType::eAccelerationStructureKHR, m_logicalDevice->getMaxFramesInFlight() * 4});
      poolSizes.push_back({vk::DescriptorType::eUniformBuffer, m_logicalDevice->getMaxFramesInFlight() * 4});
    }

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * 30,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
  }

  void Renderer3D::renderRenderObjectsByPipeline(const RenderInfo* renderInfo,
                                                 const std::shared_ptr<PipelineManager>& pipelineManager,
                                                 const std::shared_ptr<LightingManager>& lightingManager) const
  {
    const std::vector<std::shared_ptr<RenderObject>>* highlightedRenderObjects = nullptr;
    for (const auto& [pipelineType, objects] : m_renderObjectsToRender)
    {
      if (objects.empty())
      {
        continue;
      }

      if (pipelineType == PipelineType::objectHighlight)
      {
        highlightedRenderObjects = &objects;
        continue;
      }

      renderRenderObjects(pipelineManager, lightingManager, renderInfo, pipelineType, &objects);
    }

    if (highlightedRenderObjects)
    {
      renderRenderObjects(pipelineManager, lightingManager, renderInfo, PipelineType::objectHighlight, highlightedRenderObjects);
    }
  }

  void Renderer3D::renderSmokeSystems(const RenderInfo* renderInfo,
                                      const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    for (auto& system : m_smokeSystemsToRender)
    {
      system->update(renderInfo);
    }
    pipelineManager->renderSmokePipeline(renderInfo, &m_smokeSystemsToRender);
  }

  void Renderer3D::renderGrid(const std::shared_ptr<PipelineManager>& pipelineManager,
                              const RenderInfo* renderInfo)
  {
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, PipelineType::grid);

    const GridPushConstant gridPC {
      .viewProj = renderInfo->getProjectionMatrix() * renderInfo->viewMatrix,
      .viewPosition = renderInfo->viewPosition
    };

    pipelineManager->pushGraphicsPipelineConstants<GridPushConstant>(
      renderInfo->commandBuffer,
      PipelineType::grid,
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      gridPC
    );

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }

  void Renderer3D::renderRenderObjects(const std::shared_ptr<PipelineManager>& pipelineManager,
                                       const std::shared_ptr<LightingManager>& lightingManager,
                                       const RenderInfo* renderInfo,
                                       const PipelineType pipelineType,
                                       const std::vector<std::shared_ptr<RenderObject>>* objects) const
  {
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, pipelineType);

    bindPushConstant(pipelineManager, renderInfo->commandBuffer, pipelineType);

    bindDescriptorSets(pipelineManager, lightingManager, renderInfo->commandBuffer, pipelineType, renderInfo->currentFrame);

    for (const auto& object : *objects)
    {
      object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      pipelineManager->bindGraphicsPipelineDescriptorSet(
        renderInfo->commandBuffer,
        pipelineType,
        object->getDescriptorSet(renderInfo->currentFrame),
        0
      );

      object->draw(renderInfo->commandBuffer);
    }
  }

  void Renderer3D::bindPushConstant(const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const PipelineType pipelineType) const
  {
    const auto it = m_pushConstants.find(pipelineType);
    if (it == m_pushConstants.end())
    {
      return;
    }

    std::visit([&]<typename T>(const T& pc) {
      pipelineManager->pushGraphicsPipelineConstants<T>(
        commandBuffer,
        pipelineType,
        it->second.stages,
        0,
        pc
      );
    }, it->second.data);
  }

  void Renderer3D::bindDescriptorSets(const std::shared_ptr<PipelineManager>& pipelineManager,
                                      const std::shared_ptr<LightingManager>& lightingManager,
                                      const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const PipelineType pipelineType,
                                      const uint32_t currentFrame) const
  {
    if (pipelineType == PipelineType::cubeMap)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        m_cubeMapDescriptorSet->getDescriptorSet(currentFrame),
        1
      );
    }

    if (pipelineType == PipelineType::bumpyCurtain ||
        pipelineType == PipelineType::noisyEllipticalDots)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        m_noiseDescriptorSet->getDescriptorSet(currentFrame),
        2
      );
    }

    if (pipelineType == PipelineType::ellipticalDots ||
        pipelineType == PipelineType::crosses ||
        pipelineType == PipelineType::curtain ||
        pipelineType == PipelineType::bumpyCurtain ||
        pipelineType == PipelineType::object ||
        pipelineType == PipelineType::snake ||
        pipelineType == PipelineType::noisyEllipticalDots)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        lightingManager->getLightingDescriptorSet()->getDescriptorSet(currentFrame),
        1
      );
    }
  }

  void Renderer3D::createDescriptorSets()
  {
    m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, m_commandPool, "assets/noise/noise3d.064.tex",
                                                 vk::SamplerAddressMode::eRepeat);

    std::array<std::string, 6> paths {
      "assets/cubeMap/nvposx.bmp",
      "assets/cubeMap/nvnegx.bmp",
      "assets/cubeMap/nvposy.bmp",
      "assets/cubeMap/nvnegy.bmp",
      "assets/cubeMap/nvposz.bmp",
      "assets/cubeMap/nvnegz.bmp"
    };
    m_cubeMapTexture = std::make_shared<TextureCubemap>(m_logicalDevice, m_commandPool, paths);

    constexpr vk::DescriptorSetLayoutBinding noiseSamplerLayout {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    std::vector noiseLayoutBindings {
      noiseSamplerLayout
    };

    m_noiseDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, m_descriptorPool, noiseLayoutBindings);
    m_noiseDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      std::vector descriptorWrites{{
        m_noiseTexture->getDescriptorSet(0, descriptorSet)
      }};

      return descriptorWrites;
    });

    constexpr vk::DescriptorSetLayoutBinding cubeMapSamplerLayout {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    std::vector cubeMapLayoutBindings {
      noiseSamplerLayout,
      cubeMapSamplerLayout
    };

    m_cubeMapDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, m_descriptorPool, cubeMapLayoutBindings);
    m_cubeMapDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      std::vector descriptorWrites{{
        m_noiseTexture->getDescriptorSet(0, descriptorSet),
        m_cubeMapTexture->getDescriptorSet(1, descriptorSet)
      }};

      return descriptorWrites;
    });
  }

  bool Renderer3D::pipelineIsActive(const PipelineType pipelineType) const
  {
    return m_renderObjectsToRender.contains(pipelineType) && !m_renderObjectsToRender.at(pipelineType).empty();
  }

  void Renderer3D::displayGui()
  {
    displayCrossesGui();
    
    displayCurtainGui();

    displayEllipticalDotsGui();

    displayMiscGui();
  }

  void Renderer3D::displayCrossesGui()
  {
    if (pipelineIsActive(PipelineType::crosses))
    {
      auto& crossesPC = std::get<CrossesPushConstant>(m_pushConstants.at(PipelineType::crosses).data);

      ImGui::Begin("Crosses");

      ImGui::SliderInt("Level", &crossesPC.level, 0, 3);

      ImGui::SliderFloat("Quantize", &crossesPC.quantize, 2.0f, 50.0f);

      ImGui::SliderFloat("Size", &crossesPC.size, 0.0001f, 0.1f);

      ImGui::SliderFloat("Shininess", &crossesPC.shininess, 2.0f, 50.0f);

      ImGui::Separator();

      ImGui::Text("Chroma Depth");

      bool useChromaDepth = crossesPC.useChromaDepth;
      ImGui::Checkbox("Use Chroma Depth", &useChromaDepth);
      crossesPC.useChromaDepth = useChromaDepth;

      ImGui::SliderFloat("Blue Depth", &crossesPC.blueDepth, 0.0f, 50.0f);

      ImGui::SliderFloat("Red Depth", &crossesPC.redDepth, 0.0f, 50.0f);

      ImGui::End();
    }
  }

  void Renderer3D::displayCurtainGui()
  {
    if (pipelineIsActive(PipelineType::bumpyCurtain))
    {
      auto& bumpyCurtainPC = std::get<BumpyCurtainPushConstant>(m_pushConstants.at(PipelineType::bumpyCurtain).data);

      ImGui::Begin("Bumpy Curtain");

      ImGui::SliderFloat("Amplitude", &bumpyCurtainPC.amplitude, 0.001f, 3.0f);
      ImGui::SliderFloat("Period", &bumpyCurtainPC.period, 0.1f, 10.0f);
      ImGui::SliderFloat("Shininess", &bumpyCurtainPC.shininess, 1.0f, 100.0f);

      ImGui::Separator();

      ImGui::SliderFloat("Noise Amplitude", &bumpyCurtainPC.noiseAmplitude, 0.0f, 10.0f);
      ImGui::SliderFloat("Noise Frequency", &bumpyCurtainPC.noiseFrequency, 0.1f, 10.0f);

      ImGui::End();
    }

    if (pipelineIsActive(PipelineType::curtain))
    {
      auto& curtainPC = std::get<CurtainPushConstant>(m_pushConstants.at(PipelineType::curtain).data);

      ImGui::Begin("Curtain");

      ImGui::SliderFloat("Amplitude", &curtainPC.amplitude, 0.001f, 3.0f);
      ImGui::SliderFloat("Period", &curtainPC.period, 0.1f, 10.0f);
      ImGui::SliderFloat("Shininess", &curtainPC.shininess, 1.0f, 100.0f);

      ImGui::End();
    }
  }

  void Renderer3D::displayEllipticalDotsGui()
  {
    if (pipelineIsActive(PipelineType::ellipticalDots))
    {
      auto& ellipticalDotsPC = std::get<EllipticalDotsPushConstant>(m_pushConstants.at(PipelineType::ellipticalDots).data);

      ImGui::Begin("Elliptical Dots");

      ImGui::SliderFloat("Shininess", &ellipticalDotsPC.shininess, 1.0f, 25.0f);
      ImGui::SliderFloat("S Diameter", &ellipticalDotsPC.sDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("T Diameter", &ellipticalDotsPC.tDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("blendFactor", &ellipticalDotsPC.blendFactor, 0.0f, 1.0f);

      ImGui::End();
    }

    if (pipelineIsActive(PipelineType::noisyEllipticalDots))
    {
      auto& noisyEllipticalDotsPC = std::get<NoisyEllipticalDotsPushConstant>(m_pushConstants.at(PipelineType::noisyEllipticalDots).data);

      ImGui::Begin("Noisy Elliptical Dots");

      ImGui::SliderFloat("Shininess", &noisyEllipticalDotsPC.shininess, 1.0f, 25.0f);
      ImGui::SliderFloat("S Diameter", &noisyEllipticalDotsPC.sDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("T Diameter", &noisyEllipticalDotsPC.tDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("blendFactor", &noisyEllipticalDotsPC.blendFactor, 0.0f, 1.0f);

      ImGui::Separator();

      ImGui::SliderFloat("Noise Amplitude", &noisyEllipticalDotsPC.noiseAmplitude, 0.0f, 1.0f);
      ImGui::SliderFloat("Noise Frequency", &noisyEllipticalDotsPC.noiseFrequency, 0.0f, 10.0f);

      ImGui::End();
    }
  }

  void Renderer3D::displayMiscGui()
  {
    if (pipelineIsActive(PipelineType::magnifyWhirlMosaic))
    {
      auto& magnifyWhirlMosaicPC = std::get<MagnifyWhirlMosaicPushConstant>(m_pushConstants.at(PipelineType::magnifyWhirlMosaic).data);

      ImGui::Begin("Magnify Whirl Mosaic");

      ImGui::SliderFloat("Lens S Center", &magnifyWhirlMosaicPC.lensS, 0.0f, 1.0f);
      ImGui::SliderFloat("Lens T Center", &magnifyWhirlMosaicPC.lensT, 0.0f, 1.0f);
      ImGui::SliderFloat("Lens Radius", &magnifyWhirlMosaicPC.lensRadius, 0.01f, 0.75f);

      ImGui::Separator();

      ImGui::SliderFloat("Magnification", &magnifyWhirlMosaicPC.magnification, 0.1f, 7.5f);
      ImGui::SliderFloat("Whirl", &magnifyWhirlMosaicPC.whirl, -30.0f, 30.0f);
      ImGui::SliderFloat("Mosaic", &magnifyWhirlMosaicPC.mosaic, 0.001f, 0.1f);

      ImGui::End();
    }

    if (pipelineIsActive(PipelineType::snake))
    {
      auto& snakePC = std::get<SnakePushConstant>(m_pushConstants.at(PipelineType::snake).data);

      ImGui::Begin("Snake");

      ImGui::SliderFloat("Wiggle", &snakePC.wiggle, -1.0f, 1.0f);

      ImGui::End();

      static float w = 0.0f;
      w += 0.025f;

      snakePC.wiggle = sin(w);
    }

    if (pipelineIsActive(PipelineType::cubeMap))
    {
      auto& cubeMapPC = std::get<CubeMapPushConstant>(m_pushConstants.at(PipelineType::cubeMap).data);

      ImGui::Begin("Cube Map");

      ImGui::SliderFloat("Refract | Reflect -> Blend", &cubeMapPC.mix, 0.0f, 1.0f);
      ImGui::SliderFloat("Index of Refraction", &cubeMapPC.refractionIndex, 0.0f, 5.0f);
      ImGui::SliderFloat("White Mix", &cubeMapPC.whiteMix, 0.0f, 1.0f);

      ImGui::Separator();

      ImGui::SliderFloat("Noise Amplitude", &cubeMapPC.noiseAmplitude, 0.0f, 5.0f);
      ImGui::SliderFloat("Noise Frequency", &cubeMapPC.noiseFrequency, 0.0f, 0.5f);

      ImGui::End();
    }
  }

} // vke