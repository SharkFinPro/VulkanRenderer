#include "Renderer3D.h"
#include "MousePicker.h"
#include "ObjectTable.h"
#include "../../assets/AssetManager.h"
#include "../../assets/objects/Material.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../assets/textures/Texture3D.h"
#include "../../assets/textures/TextureCubemap.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../lighting/lights/Light.h"
#include "../../lighting/lights/SpotLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/implementations/LinePipeline.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../../utilities/Frustum.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <utility>

namespace {

  // The pipelines whose fragment shaders light the object, which take the lighting set at set 2
  [[nodiscard]] bool usesLightingSet(const vke::PipelineType pipelineType)
  {
    switch (pipelineType)
    {
      case vke::PipelineType::bumpyCurtain:
      case vke::PipelineType::crosses:
      case vke::PipelineType::curtain:
      case vke::PipelineType::ellipticalDots:
      case vke::PipelineType::noisyEllipticalDots:
      case vke::PipelineType::object:
      case vke::PipelineType::snake:
        return true;
      default:
        return false;
    }
  }

  // The pipelines whose vertex stage places every vertex by the model matrix alone, so the model's bounds under that
  // matrix enclose everything they draw and the main pass can cull them against the camera. Others displace vertices
  // (curtains, snake) or emit geometry from a geometry stage (crosses, snake), which can reach past the bounds; a pipeline
  // not listed here is never culled.
  [[nodiscard]] bool cullsByBounds(const vke::PipelineType pipelineType)
  {
    switch (pipelineType)
    {
      case vke::PipelineType::cubeMap:
      case vke::PipelineType::ellipticalDots:
      case vke::PipelineType::magnifyWhirlMosaic:
      case vke::PipelineType::noisyEllipticalDots:
      case vke::PipelineType::object:
      case vke::PipelineType::texturedPlane:
        return true;
      default:
        return false;
    }
  }

  [[nodiscard]] bool castsShadows(const std::shared_ptr<vke::Light>& light)
  {
    return light->castsShadows();
  }

  // How many lists the shadow maps need: one shared by the point lights if any casts shadows, one per spot light that does
  [[nodiscard]] size_t countShadowLists(const vke::LightingManager& lightingManager)
  {
    return (std::ranges::any_of(lightingManager.getPointLightsToRender(), castsShadows) ? 1 : 0) +
           static_cast<size_t>(std::ranges::count_if(lightingManager.getSpotLightsToRender(), castsShadows));
  }

  // The camera block of the scene set (SceneCamera in shaders/common/SceneData.glsl)
  struct SceneCamera {
    glm::mat4 view;
    glm::mat4 proj;
  };

  [[nodiscard]] vke::ObjectData makeObjectData(const glm::mat4& model,
                                               const uint32_t pickId)
  {
    return {
      .model = model,
      .normalMatrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model)))),
      .pickId = pickId,
      .pad0 = 0,
      .pad1 = 0,
      .pad2 = 0
    };
  }

} // namespace

namespace vke {

  Renderer3D::Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<AssetManager> assetManager)
    : m_logicalDevice(std::move(logicalDevice)), m_assetManager(std::move(assetManager))
  {
    createCommandPool();

    createDescriptorPool();

    m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice);

    createDescriptorSets();

    createSceneDescriptorSet();

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      m_rayTracer = std::make_unique<RayTracer>(m_logicalDevice, m_assetManager, m_descriptorPool);
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
    lightingManager->renderShadowMaps(commandBuffer, pipelineManager, m_shadowRuns,
                                      std::span(m_spotShadowRuns).first(m_spotShadowCount),
                                      m_sceneDescriptorSet->getDescriptorSet(currentFrame), currentFrame);
  }

  void Renderer3D::updateSceneData(const RenderInfo* renderInfo,
                                   const std::shared_ptr<LightingManager>& lightingManager)
  {
    const uint32_t currentFrame = renderInfo->currentFrame;

    const size_t objectCount = m_renderObjectsToRenderFlattened.size() + m_outlineObjects.size();

    // The most every pass's list can hold: each shadow list and the pipelines' lists take at most every submission
    // (together, the pipelines' lists exactly that many), then the pickable ones and the outlines
    const size_t slotCount = (countShadowLists(*lightingManager) + 1) * m_renderObjectsToRenderFlattened.size() +
                             m_mousePicker->m_renderObjectsToMousePick.size() + m_outlineObjects.size();

    if (m_objectTable->reserve(currentFrame, objectCount, slotCount))
    {
      // Only this frame's set: the others may still be in use by frames in flight.
      m_sceneDescriptorSet->updateDescriptorSet(currentFrame, [this](const vk::DescriptorSet descriptorSet, const size_t frame)
      {
        return getSceneDescriptorWrites(descriptorSet, frame);
      });
    }

    writeObjectTable(currentFrame);

    const RenderInfo sceneRenderInfo = makeSceneRenderInfo(renderInfo);

    const SceneCamera camera {
      .view = m_viewMatrix,
      .proj = sceneRenderInfo.getProjectionMatrix()
    };

    // The same matrices the camera block gives the shaders, so the planes are the ones the vertex stages clip against
    const Frustum cameraFrustum = Frustum::fromViewProjection(camera.proj * camera.view);

    [[maybe_unused]] const uint32_t slotsWritten = buildDrawLists(m_objectTable->getDrawSlots(currentFrame),
                                                                  cameraFrustum, *lightingManager);
    assert(slotsWritten <= m_objectTable->getDrawSlotCapacity(currentFrame));

    m_cameraUniform->update(currentFrame, &camera);
  }

  void Renderer3D::writeObjectTable(const uint32_t currentFrame)
  {
    ObjectData* entries = m_objectTable->getEntries(currentFrame);

    m_slotKeys.clear();
    m_slotBounds.clear();

    for (size_t slot = 0; slot < m_renderObjectsToRenderFlattened.size(); ++slot)
    {
      writeObjectEntry(entries[slot], *m_renderObjectsToRenderFlattened[slot], m_pickIds[slot]);
    }

    // The outlines' entries follow the render submissions'
    entries += m_renderObjectsToRenderFlattened.size();

    for (size_t i = 0; i < m_outlineObjects.size(); ++i)
    {
      writeObjectEntry(entries[i], *m_outlineObjects[i].first, 0);
    }
  }

  void Renderer3D::writeObjectEntry(ObjectData& entry,
                                    const RenderObject& object,
                                    const uint32_t pickId)
  {
    const glm::mat4 model = object.getModelMatrix();

    entry = makeObjectData(model, pickId);

    m_slotKeys.push_back({ object.getModel().get(), object.getMaterial().get() });

    m_slotBounds.push_back(transformAABB(object.getModel()->getBounds(), model));
  }

  uint32_t Renderer3D::buildDrawLists(uint32_t* drawSlots,
                                      const Frustum& camera,
                                      const LightingManager& lightingManager)
  {
    uint32_t offset = buildShadowRuns(drawSlots, 0, lightingManager);

    offset = buildPipelineRuns(drawSlots, offset, camera);

    offset = buildPickRuns(drawSlots, offset, camera);

    return buildOutlineRuns(drawSlots, offset, camera);
  }

  DrawItem Renderer3D::makeDrawItem(const uint32_t slot,
                                    const bool byMaterial,
                                    const uint32_t maskValue) const
  {
    const SlotKey& key = m_slotKeys[slot];

    return { key.model, byMaterial ? key.material : nullptr, maskValue, slot };
  }

  void Renderer3D::addVisibleDrawItem(const Frustum& frustum,
                                      const uint32_t slot,
                                      const bool byMaterial,
                                      const uint32_t maskValue)
  {
    if (frustum.intersects(m_slotBounds[slot]))
    {
      m_drawItems.push_back(makeDrawItem(slot, byMaterial, maskValue));
    }
  }

  uint32_t Renderer3D::buildShadowRuns(uint32_t* drawSlots,
                                       uint32_t offset,
                                       const LightingManager& lightingManager)
  {
    m_shadowRuns.clear();

    // The point lights share one list of every submission, built only if one casts shadows
    if (std::ranges::any_of(lightingManager.getPointLightsToRender(), castsShadows))
    {
      m_drawItems.clear();

      for (uint32_t slot = 0; slot < m_renderObjectsToRenderFlattened.size(); ++slot)
      {
        m_drawItems.push_back(makeDrawItem(slot, false, 0));
      }

      offset = buildDrawRuns(m_drawItems, drawSlots, offset, m_shadowRuns);
    }

    return buildSpotShadowRuns(drawSlots, offset, lightingManager);
  }

  uint32_t Renderer3D::buildSpotShadowRuns(uint32_t* drawSlots,
                                           uint32_t offset,
                                           const LightingManager& lightingManager)
  {
    m_spotShadowCount = 0;

    // The order and the filter are the ones LightingManager draws the lists in
    for (const std::shared_ptr<Light>& light : lightingManager.getSpotLightsToRender())
    {
      const auto spotLight = LightingManager::asShadowCastingSpotLight(light);

      if (!spotLight)
      {
        continue;
      }

      if (m_spotShadowCount == m_spotShadowRuns.size())
      {
        m_spotShadowRuns.emplace_back();
      }

      std::vector<DrawRun>& runs = m_spotShadowRuns[m_spotShadowCount++];
      runs.clear();

      // The matrix the shadow vertex shader multiplies by
      const Frustum lightFrustum = Frustum::fromViewProjection(spotLight->getLightViewProjectionMatrix());

      m_drawItems.clear();

      for (uint32_t slot = 0; slot < m_renderObjectsToRenderFlattened.size(); ++slot)
      {
        addVisibleDrawItem(lightFrustum, slot, false, 0);
      }

      offset = buildDrawRuns(m_drawItems, drawSlots, offset, runs);
    }

    return offset;
  }

  uint32_t Renderer3D::buildPipelineRuns(uint32_t* drawSlots,
                                         uint32_t offset,
                                         const Frustum& camera)
  {
    for (auto& [pipelineType, draws] : m_renderObjectsToRender)
    {
      m_drawItems.clear();
      draws.runs.clear();

      const bool culled = cullsByBounds(pipelineType);

      for (const uint32_t slot : draws.slots)
      {
        if (culled)
        {
          addVisibleDrawItem(camera, slot, true, 0);
        }
        else
        {
          m_drawItems.push_back(makeDrawItem(slot, true, 0));
        }
      }

      offset = buildDrawRuns(m_drawItems, drawSlots, offset, draws.runs);
    }

    return offset;
  }

  uint32_t Renderer3D::buildPickRuns(uint32_t* drawSlots,
                                     uint32_t offset,
                                     const Frustum& camera)
  {
    m_pickRuns.clear();
    m_pickAlphaTestedRuns.clear();

    for (const bool alphaTested : { false, true })
    {
      m_drawItems.clear();

      for (const auto& entry : m_mousePicker->m_renderObjectsToMousePick)
      {
        if (entry.alphaTested == alphaTested)
        {
          addVisibleDrawItem(camera, entry.slot, alphaTested, 0);
        }
      }

      offset = buildDrawRuns(m_drawItems, drawSlots, offset, alphaTested ? m_pickAlphaTestedRuns : m_pickRuns);
    }

    return offset;
  }

  uint32_t Renderer3D::buildOutlineRuns(uint32_t* drawSlots,
                                        const uint32_t offset,
                                        const Frustum& camera)
  {
    m_drawItems.clear();
    m_outlineRuns.clear();

    // The outlines' table entries follow the render submissions'
    const auto firstSlot = static_cast<uint32_t>(m_renderObjectsToRenderFlattened.size());

    for (uint32_t i = 0; i < m_outlineObjects.size(); ++i)
    {
      addVisibleDrawItem(camera, firstSlot + i, false, m_outlineObjects[i].second);
    }

    return buildDrawRuns(m_drawItems, drawSlots, offset, m_outlineRuns);
  }

  void Renderer3D::renderMousePicking(const RenderInfo* renderInfo,
                                      const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    const RenderInfo renderInfoMousePicking {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent,
      .fieldOfView = m_fieldOfView,
      .nearPlane = m_nearPlane,
      .farPlane = m_farPlane
    };

    m_mousePicker->render(&renderInfoMousePicking, pipelineManager,
                          m_sceneDescriptorSet->getDescriptorSet(renderInfo->currentFrame), m_pickRuns, m_pickAlphaTestedRuns);
  }

  void Renderer3D::recordMousePickingReadback(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                              const vk::Image image,
                                              const uint32_t currentFrame) const
  {
    m_mousePicker->recordReadback(commandBuffer, image, currentFrame);
  }

  void Renderer3D::resolveMousePicking(const uint32_t currentFrame) const
  {
    m_mousePicker->resolveReadback(currentFrame);
  }

  bool Renderer3D::hasOutlines() const
  {
    return !m_outlineObjects.empty();
  }

  void Renderer3D::renderOutlineMask(const RenderInfo* renderInfo,
                                     const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, PipelineType::outlineMask);

    pipelineManager->bindGraphicsPipelineDescriptorSet(
      renderInfo->commandBuffer,
      PipelineType::outlineMask,
      m_sceneDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      0
    );

    // There is no depth test, so the outline of an object shows through whatever is in front of it. The runs are
    // sorted by mask value, which is pushed whenever it changes.
    uint32_t pushedMaskValue = 0;

    for (const DrawRun& run : m_outlineRuns)
    {
      if (run.maskValue != pushedMaskValue)
      {
        pushedMaskValue = run.maskValue;

        pipelineManager->pushGraphicsPipelineConstants<uint32_t>(
          renderInfo->commandBuffer,
          PipelineType::outlineMask,
          vk::ShaderStageFlagBits::eFragment,
          0,
          pushedMaskValue
        );
      }

      run.model->draw(renderInfo->commandBuffer, run.firstInstance, run.instanceCount);
    }
  }

  void Renderer3D::renderOutlines(const RenderInfo* renderInfo,
                                  const std::shared_ptr<PipelineManager>& pipelineManager,
                                  const vk::DescriptorSet maskDescriptorSet) const
  {
    std::array<glm::vec4, s_maxOutlineColors + 1> colors{};
    std::ranges::copy(m_outlineColors, colors.begin());

    m_outlineColorsUniform->update(renderInfo->currentFrame, colors.data());

    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, PipelineType::outline);

    pipelineManager->bindGraphicsPipelineDescriptorSet(renderInfo->commandBuffer, PipelineType::outline, maskDescriptorSet, 0);

    pipelineManager->bindGraphicsPipelineDescriptorSet(
      renderInfo->commandBuffer,
      PipelineType::outline,
      m_outlineColorsDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      1
    );

    pipelineManager->pushGraphicsPipelineConstants<int32_t>(
      renderInfo->commandBuffer,
      PipelineType::outline,
      vk::ShaderStageFlagBits::eFragment,
      0,
      static_cast<int32_t>(std::lround(m_outlineWidth))
    );

    renderInfo->commandBuffer->draw(3, 1, 0, 0);
  }

  void Renderer3D::render(const RenderInfo* renderInfo,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          const std::shared_ptr<LightingManager>& lightingManager)
  {
    displayGui();

    const RenderInfo renderInfo3D = makeSceneRenderInfo(renderInfo);

    auto& cubeMapPC = std::get<CubeMapPushConstant>(m_pushConstants.at(PipelineType::cubeMap).data);
    cubeMapPC.position = renderInfo3D.viewPosition;

    renderRenderObjectsByPipeline(&renderInfo3D, pipelineManager, lightingManager);

    pipelineManager->renderBendyPlantPipeline(&renderInfo3D, &m_bendyPlantsToRender);

    renderSmokeSystems(&renderInfo3D, pipelineManager);

    renderLinesAndGrid(&renderInfo3D, pipelineManager);
  }

  void Renderer3D::renderViewOnly(const RenderInfo* renderInfo,
                                  const std::shared_ptr<PipelineManager>& pipelineManager,
                                  const std::shared_ptr<LightingManager>& lightingManager)
  {
    const RenderInfo renderInfo3D = makeSceneRenderInfo(renderInfo);

    for (const auto& [pipelineType, draws] : m_renderObjectsToRender)
    {
      if (draws.runs.empty() || !isViewOnlyPipeline(pipelineType))
      {
        continue;
      }

      renderRenderObjects(pipelineManager, lightingManager, &renderInfo3D, pipelineType, draws.runs);
    }

    renderLinesAndGrid(&renderInfo3D, pipelineManager);
  }

  RenderInfo Renderer3D::makeSceneRenderInfo(const RenderInfo* renderInfo) const
  {
    return {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent,
      .fieldOfView = m_fieldOfView,
      .nearPlane = m_nearPlane,
      .farPlane = m_farPlane
    };
  }

  void Renderer3D::buildLineBatches()
  {
    m_lineBatches = m_linesToRender;
    for (const auto& [line, depth, _] : m_timedLines)
    {
      m_lineBatches[static_cast<size_t>(depth)].push_back(line);
    }
  }

  void Renderer3D::renderLinesAndGrid(const RenderInfo* renderInfo,
                                      const std::shared_ptr<PipelineManager>& pipelineManager)
  {
    buildLineBatches();

    pipelineManager->renderLinePipeline(renderInfo, &m_lineBatches);

    if (m_shouldRenderGrid)
    {
      renderGrid(pipelineManager, renderInfo);
    }
  }

  void Renderer3D::renderOnTopLines(const RenderInfo* renderInfo,
                                    const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    const RenderInfo renderInfoLines {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent,
      .fieldOfView = m_fieldOfView,
      .nearPlane = m_nearPlane,
      .farPlane = m_farPlane
    };

    pipelineManager->renderOnTopLinePipeline(&renderInfoLines, m_lineBatches[static_cast<size_t>(DebugDepth::onTop)].size());
  }

  bool Renderer3D::doRayTracing(const RenderInfo* renderInfo,
                                const std::shared_ptr<PipelineManager>& pipelineManager,
                                const std::shared_ptr<LightingManager>& lightingManager,
                                const ImageResource& imageResource,
                                const ImageResource& depthImageResource) const
  {
    const RenderInfo renderInfoRayTracing = makeSceneRenderInfo(renderInfo);

    return m_rayTracer->doRayTracing(
      &renderInfoRayTracing,
      pipelineManager,
      lightingManager,
      imageResource,
      depthImageResource,
      m_rayTracedRenderObjects,
      m_cloudToRender
    );
  }

  void Renderer3D::renderRayTracingComposite(const RenderInfo* renderInfo,
                                             const std::shared_ptr<PipelineManager>& pipelineManager,
                                             const vk::DescriptorSet rayTracingImagesDescriptorSet) const
  {
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, PipelineType::rayTracingComposite);

    pipelineManager->bindGraphicsPipelineDescriptorSet(
      renderInfo->commandBuffer,
      PipelineType::rayTracingComposite,
      rayTracingImagesDescriptorSet,
      0
    );

    renderInfo->commandBuffer->draw(3, 1, 0, 0);
  }

  void Renderer3D::createNewFrame()
  {
    for (auto& [_, draws] : m_renderObjectsToRender)
    {
      draws.slots.clear();
      draws.runs.clear();
    }

    m_shadowRuns.clear();
    m_spotShadowCount = 0;
    m_pickRuns.clear();
    m_pickAlphaTestedRuns.clear();
    m_outlineRuns.clear();

    m_renderObjectsToRenderFlattened.clear();
    m_pickIds.clear();
    m_rayTracedRenderObjects.clear();

    m_outlineObjects.clear();
    m_outlineColors.clear();

    for (auto& batch : m_linesToRender)
    {
      batch.clear();
    }

    const auto now = std::chrono::steady_clock::now();
    std::erase_if(m_timedLines, [now](const TimedLine& timedLine) {
      return timedLine.expiry <= now;
    });

    m_bendyPlantsToRender.clear();

    m_mousePicker->clearObjectsToMousePick();

    m_smokeSystemsToRender.clear();

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

  void Renderer3D::setProjectionParameters(const float fieldOfViewDegrees,
                                           const float nearPlane,
                                           const float farPlane)
  {
    // Infinite or NaN values would make the projection, and the ray tracer's inverse of it, NaN.
    if (!std::isfinite(fieldOfViewDegrees) || !(fieldOfViewDegrees > 0.0f && fieldOfViewDegrees < 180.0f))
    {
      throw std::invalid_argument("field of view must be between 0 and 180 degrees");
    }

    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) || !(nearPlane > 0.0f && farPlane > nearPlane))
    {
      throw std::invalid_argument("clip planes must be finite and satisfy 0 < near < far");
    }

    m_fieldOfView = fieldOfViewDegrees;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
  }

  std::shared_ptr<MousePicker> Renderer3D::getMousePicker() const
  {
    return m_mousePicker;
  }

  void Renderer3D::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                const PipelineType pipelineType,
                                bool* mousePicked)
  {
    const auto slot = static_cast<uint32_t>(m_renderObjectsToRenderFlattened.size());

    m_renderObjectsToRender[pipelineType].slots.push_back(slot);

    m_renderObjectsToRenderFlattened.push_back(renderObject);

    m_pickIds.push_back(0);

    if (!isViewOnlyPipeline(pipelineType))
    {
      m_rayTracedRenderObjects.push_back(renderObject);
    }

    if (mousePicked)
    {
      m_pickIds[slot] = m_mousePicker->renderObject(renderObject, mousePicked, usesAlphaCutout(pipelineType), slot);
    }
  }

  void Renderer3D::renderPickableObject(const std::shared_ptr<RenderObject>& renderObject,
                                        const PipelineType pipelineType)
  {
    this->renderObject(renderObject, pipelineType);

    const auto slot = static_cast<uint32_t>(m_renderObjectsToRenderFlattened.size() - 1);

    m_pickIds[slot] = m_mousePicker->renderObject(renderObject, nullptr, usesAlphaCutout(pipelineType), slot);
  }

  std::optional<PickResult> Renderer3D::getPickResult() const
  {
    return m_mousePicker->getPickResult();
  }

  void Renderer3D::renderOutline(const std::shared_ptr<RenderObject>& renderObject,
                                 const glm::vec4 color)
  {
    auto colorIt = std::ranges::find(m_outlineColors, color);

    if (colorIt == m_outlineColors.end())
    {
      if (m_outlineColors.size() >= s_maxOutlineColors)
      {
        // A selection effect shouldn't take the application down, so extra colors share the last one.
        if (!m_warnedAboutOutlineColors)
        {
          std::cerr << "More than " << s_maxOutlineColors << " outline colors in a frame; extra colors use the last one" << std::endl;
          m_warnedAboutOutlineColors = true;
        }

        colorIt = std::prev(m_outlineColors.end());
      }
      else
      {
        colorIt = m_outlineColors.insert(m_outlineColors.end(), color);
      }
    }

    m_outlineObjects.emplace_back(renderObject, static_cast<uint32_t>(colorIt - m_outlineColors.begin()) + 1);
  }

  void Renderer3D::setOutlineWidth(const float pixels)
  {
    if (std::isnan(pixels))
    {
      return;
    }

    m_outlineWidth = std::clamp(pixels, 1.0f, 8.0f);
  }

  void Renderer3D::renderLine(const glm::vec3 start,
                              const glm::vec3 end,
                              const glm::vec4 color,
                              const float width,
                              const float lifetimeSeconds,
                              const DebugDepth depth)
  {
    const LineInstance line { start, end, color, width };

    // An enumerator outside the three draws like the default
    const auto mode = static_cast<size_t>(depth) < s_debugDepthModeCount ? depth : DebugDepth::tested;

    if (!(lifetimeSeconds > 0.0f))
    {
      m_linesToRender[static_cast<size_t>(mode)].push_back(line);
      return;
    }

    // Kept lines beyond what can be drawn are dropped oldest first, so the list can't grow without bound.
    if (m_timedLines.size() >= LinePipeline::s_maxLines)
    {
      m_timedLines.pop_front();
    }

    // Clamped so an enormous or infinite lifetime can't overflow the clock's integer duration.
    constexpr float maxLifetimeSeconds = 1.0e6f;

    m_timedLines.push_back({
      line,
      mode,
      std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(std::min(lifetimeSeconds, maxLifetimeSeconds)))
    });
  }

  void Renderer3D::renderBox(const glm::mat4& transform, const glm::vec3 halfExtents, const DebugStyle& style)
  {
    debugShapes::appendBox(m_shapeSegments, transform, halfExtents);
    renderShapeSegments(style);
  }

  void Renderer3D::renderBox(const glm::vec3 min, const glm::vec3 max, const DebugStyle& style)
  {
    renderBox(glm::translate(glm::mat4(1.0f), (min + max) * 0.5f), (max - min) * 0.5f, style);
  }

  void Renderer3D::renderSphere(const glm::vec3 center, const float radius, const DebugStyle& style)
  {
    debugShapes::appendSphere(m_shapeSegments, center, radius);
    renderShapeSegments(style);
  }

  void Renderer3D::renderCapsule(const glm::vec3 a, const glm::vec3 b, const float radius, const DebugStyle& style)
  {
    debugShapes::appendCapsule(m_shapeSegments, a, b, radius);
    renderShapeSegments(style);
  }

  void Renderer3D::renderCylinder(const glm::vec3 a, const glm::vec3 b, const float radius, const DebugStyle& style)
  {
    debugShapes::appendCylinder(m_shapeSegments, a, b, radius);
    renderShapeSegments(style);
  }

  void Renderer3D::renderCone(const glm::vec3 apex, const glm::vec3 baseCenter, const float radius, const DebugStyle& style)
  {
    debugShapes::appendCone(m_shapeSegments, apex, baseCenter, radius);
    renderShapeSegments(style);
  }

  void Renderer3D::renderArrow(const glm::vec3 from, const glm::vec3 to, const DebugStyle& style)
  {
    debugShapes::appendArrow(m_shapeSegments, from, to);
    renderShapeSegments(style);
  }

  void Renderer3D::renderFrustum(const glm::mat4& viewProjection, const DebugStyle& style)
  {
    debugShapes::appendFrustum(m_shapeSegments, viewProjection);
    renderShapeSegments(style);
  }

  void Renderer3D::renderAxes(const glm::mat4& transform,
                              const float length,
                              const float width,
                              const float lifetimeSeconds,
                              const DebugDepth depth)
  {
    bool finite = std::isfinite(length);
    for (int column = 0; column < 4; ++column)
    {
      for (int row = 0; row < 4; ++row)
      {
        finite = finite && std::isfinite(transform[column][row]);
      }
    }

    if (!finite)
    {
      return;
    }

    const auto origin = glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    const std::array<glm::vec3, 3> directions {{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }};
    for (const auto& direction : directions)
    {
      const auto end = glm::vec3(transform * glm::vec4(direction * length, 1.0f));
      renderLine(origin, end, { direction, 1.0f }, width, lifetimeSeconds, depth);
    }
  }

  void Renderer3D::renderShapeSegments(const DebugStyle& style)
  {
    for (const auto& [start, end] : m_shapeSegments)
    {
      renderLine(start, end, style.color, style.width, style.lifetimeSeconds, style.depth);
    }

    m_shapeSegments.clear();
  }

  void Renderer3D::clearLines()
  {
    m_timedLines.clear();
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

  vk::DescriptorSetLayout Renderer3D::getSceneDescriptorSetLayout() const
  {
    return m_sceneDescriptorSet->getDescriptorSetLayout();
  }

  vk::DescriptorSetLayout Renderer3D::getNoiseDescriptorSetLayout() const
  {
    return m_noiseDescriptorSet->getDescriptorSetLayout();
  }

  vk::DescriptorSetLayout Renderer3D::getCubeMapDescriptorSetLayout() const
  {
    return m_cubeMapDescriptorSet->getDescriptorSetLayout();
  }

  vk::DescriptorSetLayout Renderer3D::getOutlineColorsDescriptorSetLayout() const
  {
    return m_outlineColorsDescriptorSet->getDescriptorSetLayout();
  }

  void Renderer3D::setCloudToRender(std::shared_ptr<Cloud> cloud)
  {
    m_cloudToRender = std::move(cloud);
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
      {vk::DescriptorType::eStorageBuffer, m_logicalDevice->getMaxFramesInFlight() * 14},
    }};

    poolSizes.push_back({vk::DescriptorType::eUniformBuffer, m_logicalDevice->getMaxFramesInFlight() * 7});

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      poolSizes.push_back({vk::DescriptorType::eAccelerationStructureKHR, m_logicalDevice->getMaxFramesInFlight() * 4});
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
    for (const auto& [pipelineType, draws] : m_renderObjectsToRender)
    {
      if (draws.runs.empty())
      {
        continue;
      }

      renderRenderObjects(pipelineManager, lightingManager, renderInfo, pipelineType, draws.runs);
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
                                       const std::vector<DrawRun>& runs) const
  {
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, pipelineType);

    bindPushConstant(pipelineManager, renderInfo->commandBuffer, pipelineType);

    bindDescriptorSets(pipelineManager, lightingManager, renderInfo->commandBuffer, pipelineType, renderInfo->currentFrame);

    // The runs are sorted by material, so a material's set is bound once however many models use it
    const Material* boundMaterial = nullptr;

    for (const DrawRun& run : runs)
    {
      if (run.material != boundMaterial)
      {
        boundMaterial = run.material;

        pipelineManager->bindGraphicsPipelineDescriptorSet(
          renderInfo->commandBuffer,
          pipelineType,
          boundMaterial->getDescriptorSet(renderInfo->currentFrame),
          1
        );
      }

      run.model->draw(renderInfo->commandBuffer, run.firstInstance, run.instanceCount);
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
    // Set 0 is the scene, set 1 the material (bound per object). Rebound after every pipeline bind, since pipeline
    // layouts differ in their push constant ranges, which makes earlier sets incompatible.
    pipelineManager->bindGraphicsPipelineDescriptorSet(
      commandBuffer,
      pipelineType,
      m_sceneDescriptorSet->getDescriptorSet(currentFrame),
      0
    );

    if (pipelineType == PipelineType::cubeMap)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        m_cubeMapDescriptorSet->getDescriptorSet(currentFrame),
        2
      );
    }

    if (pipelineType == PipelineType::bumpyCurtain ||
        pipelineType == PipelineType::noisyEllipticalDots)
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        m_noiseDescriptorSet->getDescriptorSet(currentFrame),
        3
      );
    }

    if (usesLightingSet(pipelineType))
    {
      pipelineManager->bindGraphicsPipelineDescriptorSet(
        commandBuffer,
        pipelineType,
        lightingManager->getLightingDescriptorSet()->getDescriptorSet(currentFrame),
        2
      );
    }
  }

  void Renderer3D::createSceneDescriptorSet()
  {
    m_objectTable = std::make_shared<ObjectTable>(m_logicalDevice);

    m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SceneCamera));

    constexpr vk::DescriptorSetLayoutBinding objectTableLayout {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex |
                    vk::ShaderStageFlagBits::eGeometry |
                    vk::ShaderStageFlagBits::eFragment
    };

    constexpr vk::DescriptorSetLayoutBinding cameraLayout {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex |
                    vk::ShaderStageFlagBits::eGeometry |
                    vk::ShaderStageFlagBits::eFragment
    };

    constexpr vk::DescriptorSetLayoutBinding drawSlotsLayout {
      .binding = 2,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    m_sceneDescriptorSet = std::make_shared<DescriptorSet>(
      m_logicalDevice, m_descriptorPool, std::vector{ objectTableLayout, cameraLayout, drawSlotsLayout });
    m_sceneDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      return getSceneDescriptorWrites(descriptorSet, frame);
    });
  }

  std::array<vk::WriteDescriptorSet, 3> Renderer3D::getSceneDescriptorWrites(const vk::DescriptorSet descriptorSet,
                                                                             const size_t frame) const
  {
    const vk::WriteDescriptorSet objectTableWrite {
      .dstSet = descriptorSet,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &m_objectTable->getBufferInfo(static_cast<uint32_t>(frame))
    };

    const vk::WriteDescriptorSet drawSlotsWrite {
      .dstSet = descriptorSet,
      .dstBinding = 2,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &m_objectTable->getDrawSlotsBufferInfo(static_cast<uint32_t>(frame))
    };

    return {{ objectTableWrite, m_cameraUniform->getDescriptorSet(1, descriptorSet, frame), drawSlotsWrite }};
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

    constexpr vk::DescriptorSetLayoutBinding outlineColorsLayout {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    m_outlineColorsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(glm::vec4) * (s_maxOutlineColors + 1));

    m_outlineColorsDescriptorSet = std::make_shared<DescriptorSet>(
      m_logicalDevice, m_descriptorPool, std::vector{ outlineColorsLayout });
    m_outlineColorsDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector descriptorWrites{{
        m_outlineColorsUniform->getDescriptorSet(0, descriptorSet, frame)
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
    return m_renderObjectsToRender.contains(pipelineType) && !m_renderObjectsToRender.at(pipelineType).slots.empty();
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