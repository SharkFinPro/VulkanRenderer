#include "Renderer3D.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../mousePicker/MousePicker.h"

namespace vke {
  Renderer3D::Renderer3D()
  {
    // m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice, m_window, m_commandPool,
    //                                               m_assetManager->getObjectDescriptorSetLayout());
  }

  void Renderer3D::render(const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    // pipelineManager->renderGraphicsPipelines(m_offscreenCommandBuffer, m_offscreenViewportExtent,
    //                                          currentFrame, m_viewPosition, m_viewMatrix, m_shouldRenderGrid);
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

    if (mousePicked == nullptr)
    {
      return;
    }

    // m_mousePicker->renderObject(renderObject, mousePicked);
  }

  void Renderer3D::renderLine(glm::vec3 start,
                              glm::vec3 end)
  {
    m_lineVerticesToRender.push_back({start});
    m_lineVerticesToRender.push_back({end});
  }

  void Renderer3D::renderBendyPlant(const BendyPlant& bendyPlant)
  {
    m_bendyPlantsToRender.push_back(bendyPlant);
  }

  void Renderer3D::renderRenderObjects(const RenderInfo& renderInfo) const
  {
    // for (const auto& [type, objects] : m_renderObjectsToRender)
    // {
    //   if (objects.empty())
    //   {
    //     continue;
    //   }
    //
    //   if (auto it = m_pipelines.find(type); it != m_pipelines.end())
    //   {
    //     if (it->first == PipelineType::objectHighlight)
    //     {
    //       continue;
    //     }
    //
    //     if (auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(it->second.get()))
    //     {
    //       graphicsPipeline->displayGui();
    //       graphicsPipeline->render(&renderInfo, &objects);
    //       continue;
    //     }
    //
    //     throw std::runtime_error("Pipeline for object type is not a GraphicsPipeline");
    //   }
    //
    //   throw std::runtime_error("Pipeline for object type does not exist");
    // }
    //
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