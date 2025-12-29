#include "Renderer3D.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../mousePicker/MousePicker.h"

namespace vke {
  Renderer3D::Renderer3D()
  {
    // m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice, m_window, m_commandPool,
    //                                               m_assetManager->getObjectDescriptorSetLayout());
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

    renderRenderObjects(&renderInfo3D, pipelineManager);

    pipelineManager->renderBendyPlantPipeline(renderInfo3D, &m_bendyPlantsToRender);

    if (m_shouldRenderGrid)
    {
      pipelineManager->renderGridPipeline(&renderInfo3D);
    }
  }

  void Renderer3D::createNewFrame()
  {
    for (auto& [_, objects] : m_renderObjectsToRender)
    {
      objects.clear();
    }

    m_lineVerticesToRender.clear();

    m_bendyPlantsToRender.clear();

    // m_mousePicker->clearObjectsToMousePick();
  }

  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& Renderer3D::getRenderObjectsToRender()
  {
    return m_renderObjectsToRender;
  }

  void Renderer3D::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                PipelineType pipelineType,
                                bool* mousePicked)
  {
    m_renderObjectsToRender[pipelineType].push_back(renderObject);

    if (mousePicked)
    {
      // m_mousePicker->renderObject(renderObject, mousePicked);
    }
  }

  void Renderer3D::renderLine(const glm::vec3 start,
                              const glm::vec3 end)
  {
    m_lineVerticesToRender.push_back({start});
    m_lineVerticesToRender.push_back({end});
  }

  void Renderer3D::renderBendyPlant(const BendyPlant& bendyPlant)
  {
    m_bendyPlantsToRender.push_back(bendyPlant);
  }

  void Renderer3D::renderRenderObjects(const RenderInfo* renderInfo,
                                       const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    for (const auto& [pipelineType, objects] : m_renderObjectsToRender)
    {
      if (objects.empty())
      {
        continue;
      }

      pipelineManager->renderRenderObjectPipeline(renderInfo, &objects, pipelineType);
    }

    // pipelineManager->renderObjectHighlightPipeline()

    // auto highlightObjectsIt = m_renderObjectsToRender.find(PipelineType::objectHighlight);
    // auto highlightPipelineIt = m_pipelines.find(PipelineType::objectHighlight);
    //
    // if (highlightObjectsIt != m_renderObjectsToRender.end() &&
    //     highlightPipelineIt != m_pipelines.end())
    // {
    //   auto& highlightObjects = highlightObjectsIt->second;
    //
    //   if (!highlightObjects.empty())
    //   {
    //     if (auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(highlightPipelineIt->second.get()))
    //     {
    //       graphicsPipeline->displayGui();
    //       graphicsPipeline->render(&renderInfo, &highlightObjects);
    //     }
    //   }
    // }
  }

  void Renderer3D::renderSmokeSystems(const RenderInfo& renderInfo) const
  {
    // if (!m_smokeSystems.empty())
    // {
    //   ImGui::Begin("Smoke");
    //   ImGui::Separator();
    //   for (const auto& system : m_smokeSystems)
    //   {
    //     ImGui::PushID(&system);
    //     system->displayGui();
    //     ImGui::PopID();
    //
    //     ImGui::Separator();
    //
    //     system->render(&renderInfo, nullptr);
    //   }
    //   ImGui::End();
    // }
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
} // vke