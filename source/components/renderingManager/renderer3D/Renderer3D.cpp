#include "Renderer3D.h"
#include "MousePicker.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../assets/textures/Texture3D.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"

namespace vke {
  Renderer3D::Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<Window> window)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice, std::move(window), m_commandPool);

    m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, m_commandPool, "assets/noise/noise3d.064.tex",
                                                 VK_SAMPLER_ADDRESS_MODE_REPEAT);

    constexpr VkDescriptorSetLayoutBinding noiseSamplerLayout {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    std::vector<VkDescriptorSetLayoutBinding> noiseLayoutBindings {
      noiseSamplerLayout
    };

    m_noiseDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, m_descriptorPool, noiseLayoutBindings);
    m_noiseDescriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_noiseTexture->getDescriptorSet(0, descriptorSet)
      }};

      return descriptorWrites;
    });
  }

  Renderer3D::~Renderer3D()
  {
    m_logicalDevice->destroyDescriptorPool(m_descriptorPool);

    m_logicalDevice->destroyCommandPool(m_commandPool);
  }

  void Renderer3D::renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const uint32_t currentFrame) const
  {
    lightingManager->update(currentFrame, m_viewPosition);

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

  void Renderer3D::handleRenderedMousePickingImage(VkImage image) const
  {
    m_mousePicker->handleRenderedMousePickingImage(image);
  }

  void Renderer3D::render(const RenderInfo* renderInfo,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          const std::shared_ptr<LightingManager>& lightingManager)
  {
    if (m_renderObjectsToRender.contains(PipelineType::magnifyWhirlMosaic) &&
        !m_renderObjectsToRender.at(PipelineType::magnifyWhirlMosaic).empty())
    {
      ImGui::Begin("Magnify Whirl Mosaic");

      ImGui::SliderFloat("Lens S Center", &m_magnifyWhirlMosaicPC.lensS, 0.0f, 1.0f);
      ImGui::SliderFloat("Lens T Center", &m_magnifyWhirlMosaicPC.lensT, 0.0f, 1.0f);
      ImGui::SliderFloat("Lens Radius", &m_magnifyWhirlMosaicPC.lensRadius, 0.01f, 0.75f);

      ImGui::Separator();

      ImGui::SliderFloat("Magnification", &m_magnifyWhirlMosaicPC.magnification, 0.1f, 7.5f);
      ImGui::SliderFloat("Whirl", &m_magnifyWhirlMosaicPC.whirl, -30.0f, 30.0f);
      ImGui::SliderFloat("Mosaic", &m_magnifyWhirlMosaicPC.mosaic, 0.001f, 0.1f);

      ImGui::End();
    }

    if (m_renderObjectsToRender.contains(PipelineType::ellipticalDots) &&
        !m_renderObjectsToRender.at(PipelineType::ellipticalDots).empty())
    {
      ImGui::Begin("Elliptical Dots");

      ImGui::SliderFloat("Shininess", &m_ellipticalDotsPC.shininess, 1.0f, 25.0f);
      ImGui::SliderFloat("S Diameter", &m_ellipticalDotsPC.sDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("T Diameter", &m_ellipticalDotsPC.tDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("blendFactor", &m_ellipticalDotsPC.blendFactor, 0.0f, 1.0f);

      ImGui::End();
    }

    if (m_renderObjectsToRender.contains(PipelineType::crosses) &&
        !m_renderObjectsToRender.at(PipelineType::crosses).empty())
    {
      ImGui::Begin("Crosses");

      ImGui::SliderInt("Level", &m_crossesPC.level, 0, 3);

      ImGui::SliderFloat("Quantize", &m_crossesPC.quantize, 2.0f, 50.0f);

      ImGui::SliderFloat("Size", &m_crossesPC.size, 0.0001f, 0.1f);

      ImGui::SliderFloat("Shininess", &m_crossesPC.shininess, 2.0f, 50.0f);

      ImGui::Separator();

      ImGui::Text("Chroma Depth");

      bool useChromaDepth = m_crossesPC.useChromaDepth;
      ImGui::Checkbox("Use Chroma Depth", &useChromaDepth);
      m_crossesPC.useChromaDepth = useChromaDepth;

      ImGui::SliderFloat("Blue Depth", &m_crossesPC.blueDepth, 0.0f, 50.0f);

      ImGui::SliderFloat("Red Depth", &m_crossesPC.redDepth, 0.0f, 50.0f);

      ImGui::End();
    }

    if (m_renderObjectsToRender.contains(PipelineType::curtain) &&
        !m_renderObjectsToRender.at(PipelineType::curtain).empty())
    {
      ImGui::Begin("Curtain");

      ImGui::SliderFloat("Amplitude", &m_curtainPC.amplitude, 0.001f, 3.0f);
      ImGui::SliderFloat("Period", &m_curtainPC.period, 0.1f, 10.0f);
      ImGui::SliderFloat("Shininess", &m_curtainPC.shininess, 1.0f, 100.0f);

      ImGui::End();
    }

    if (m_renderObjectsToRender.contains(PipelineType::snake) &&
        !m_renderObjectsToRender.at(PipelineType::snake).empty())
    {
      ImGui::Begin("Snake");

      ImGui::SliderFloat("Wiggle", &m_snakePC.wiggle, -1.0f, 1.0f);

      ImGui::End();

      static float w = 0.0f;
      w += 0.025f;

      m_snakePC.wiggle = sin(w);
    }

    if (m_renderObjectsToRender.contains(PipelineType::noisyEllipticalDots) &&
        !m_renderObjectsToRender.at(PipelineType::noisyEllipticalDots).empty())
    {
      ImGui::Begin("Noisy Elliptical Dots");

      ImGui::SliderFloat("Shininess", &m_noisyEllipticalDotsPC.shininess, 1.0f, 25.0f);
      ImGui::SliderFloat("S Diameter", &m_noisyEllipticalDotsPC.sDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("T Diameter", &m_noisyEllipticalDotsPC.tDiameter, 0.001f, 0.5f);
      ImGui::SliderFloat("blendFactor", &m_noisyEllipticalDotsPC.blendFactor, 0.0f, 1.0f);

      ImGui::Separator();

      ImGui::SliderFloat("Noise Amplitude", &m_noisyEllipticalDotsPC.amplitude, 0.0f, 1.0f);
      ImGui::SliderFloat("Noise Frequency", &m_noisyEllipticalDotsPC.frequency, 0.0f, 10.0f);

      ImGui::End();
    }

    const RenderInfo renderInfo3D {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent
    };

    renderRenderObjectsByPipeline(&renderInfo3D, pipelineManager, lightingManager);

    pipelineManager->renderBendyPlantPipeline(&renderInfo3D, &m_bendyPlantsToRender);

    renderSmokeSystems(&renderInfo3D, pipelineManager);

    pipelineManager->renderLinePipeline(&renderInfo3D, &m_lineVerticesToRender);

    if (m_shouldRenderGrid)
    {
      renderGrid(pipelineManager, &renderInfo3D);
    }
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

  VkDescriptorSetLayout Renderer3D::getNoiseDescriptorSetLayout() const
  {
    return m_noiseDescriptorSet->getDescriptorSetLayout();
  }

  void Renderer3D::createCommandPool()
  {
    const VkCommandPoolCreateInfo poolInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void Renderer3D::createDescriptorPool()
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

      if (pipelineType == PipelineType::bumpyCurtain ||
          pipelineType == PipelineType::cubeMap)
      {
        pipelineManager->renderRenderObjectPipeline(renderInfo, &objects, pipelineType);
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

    pipelineManager->pushGraphicsPipelineConstants(
      renderInfo->commandBuffer,
      PipelineType::grid,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(gridPC),
      &gridPC
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

    if (pipelineType == PipelineType::magnifyWhirlMosaic)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_magnifyWhirlMosaicPC),
        &m_magnifyWhirlMosaicPC
      );
    }

    if (pipelineType == PipelineType::ellipticalDots)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_ellipticalDotsPC),
        &m_ellipticalDotsPC
      );
    }

    if (pipelineType == PipelineType::crosses)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_crossesPC),
        &m_crossesPC
      );
    }

    if (pipelineType == PipelineType::curtain)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_curtainPC),
        &m_curtainPC
      );
    }

    if (pipelineType == PipelineType::snake)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_snakePC),
        &m_snakePC
      );
    }

    if (pipelineType == PipelineType::noisyEllipticalDots)
    {
      pipelineManager->pushGraphicsPipelineConstants(
        renderInfo->commandBuffer,
        pipelineType,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(m_noisyEllipticalDotsPC),
        &m_noisyEllipticalDotsPC
      );

      pipelineManager->bindGraphicsPipelineDescriptorSet(
        renderInfo->commandBuffer,
        pipelineType,
        m_noiseDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
        2
      );
    }

    if (pipelineType == PipelineType::ellipticalDots ||
        pipelineType == PipelineType::crosses ||
        pipelineType == PipelineType::curtain ||
        pipelineType == PipelineType::object ||
        pipelineType == PipelineType::snake ||
        pipelineType == PipelineType::noisyEllipticalDots)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        renderInfo->commandBuffer,
        pipelineType,
        lightingManager->getLightingDescriptorSet()->getDescriptorSet(renderInfo->currentFrame),
        1
      );
    }

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
} // vke