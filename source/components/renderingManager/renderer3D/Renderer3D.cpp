#include "Renderer3D.h"
#include "MousePicker.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/particleSystems/SmokeSystem.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"

namespace vke {
  Renderer3D::Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<Window> window,
                         VkCommandPool commandPool)
    : m_mousePicker(std::make_shared<MousePicker>(std::move(logicalDevice), std::move(window), commandPool))
  {}

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
                          const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    const RenderInfo renderInfo3D {
      .commandBuffer = renderInfo->commandBuffer,
      .currentFrame = renderInfo->currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = renderInfo->extent
    };

    renderRenderObjectsByPipeline(&renderInfo3D, pipelineManager);

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

  void Renderer3D::renderRenderObjectsByPipeline(const RenderInfo* renderInfo,
                                                 const std::shared_ptr<PipelineManager>& pipelineManager) const
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

      if (pipelineType == PipelineType::texturedPlane)
      {
        renderRenderObjects(pipelineManager, renderInfo, PipelineType::texturedPlane);
        continue;
      }

      pipelineManager->renderRenderObjectPipeline(renderInfo, &objects, pipelineType);
    }

    if (highlightedRenderObjects)
    {
      pipelineManager->renderRenderObjectPipeline(renderInfo, highlightedRenderObjects, PipelineType::objectHighlight);
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
    pipelineManager->bindGridPipeline(renderInfo->commandBuffer);

    const GridPushConstant gridPC {
      .viewProj = renderInfo->getProjectionMatrix() * renderInfo->viewMatrix,
      .viewPosition = renderInfo->viewPosition
    };

    pipelineManager->pushGridPipelineConstants(
      renderInfo->commandBuffer,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(gridPC),
      &gridPC
    );

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }

  void Renderer3D::renderRenderObjects(const std::shared_ptr<PipelineManager>& pipelineManager,
                                       const RenderInfo* renderInfo,
                                       const PipelineType pipelineType) const
  {
    for (const auto& object : m_renderObjectsToRenderFlattened)
    {
      object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      pipelineManager->bindRenderObjectPipeline(renderInfo->commandBuffer, pipelineType);

      pipelineManager->bindRenderObjectPipelineDescriptorSet(
        renderInfo->commandBuffer,
        pipelineType,
        object->getDescriptorSet(renderInfo->currentFrame),
        0
      );

      object->draw(renderInfo->commandBuffer);
    }
  }
} // vke