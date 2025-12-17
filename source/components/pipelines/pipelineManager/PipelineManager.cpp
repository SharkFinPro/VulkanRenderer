#include "PipelineManager.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../lighting/LightingManager.h"
#include "../../mousePicker/MousePicker.h"

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

#include "../implementations/BendyPipeline.h"
#include "../implementations/DotsPipeline.h"
#include "../implementations/GuiPipeline.h"
#include "../implementations/SmokePipeline.h"

#include <imgui.h>
#include <ranges>

namespace vke {

PipelineManager::PipelineManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const std::shared_ptr<LightingManager>& lightingManager,
                                 const std::shared_ptr<MousePicker>& mousePicker,
                                 VkDescriptorSetLayout objectDescriptorSetLayout,
                                 VkDescriptorPool descriptorPool,
                                 VkCommandPool commandPool,
                                 const bool shouldDoDots)
  : m_logicalDevice(logicalDevice), m_commandPool(commandPool), m_descriptorPool(descriptorPool),
    m_renderPass(renderPass), m_lightingManager(lightingManager), m_mousePicker(mousePicker),
    m_shouldDoDots(shouldDoDots)
{
  createPipelines(objectDescriptorSetLayout);
}

void PipelineManager::createNewFrame()
{
  for (auto& [_, objects] : m_renderObjectsToRender)
  {
    objects.clear();
  }

  m_lineVerticesToRender.clear();

  m_bendyPipeline->clearBendyPlantsToRender();
}

void PipelineManager::renderObject(const std::shared_ptr<RenderObject>& renderObject, const PipelineType pipelineType,
                                   bool* mousePicked)
{
  m_renderObjectsToRender[pipelineType].push_back(renderObject);

  if (mousePicked == nullptr)
  {
    return;
  }

  m_mousePicker->renderObject(renderObject, mousePicked);
}

void PipelineManager::renderLine(const glm::vec3 start, const glm::vec3 end)
{
  m_lineVerticesToRender.push_back({start});
  m_lineVerticesToRender.push_back({end});
}

void PipelineManager::renderBendyPlant(const BendyPlant& bendyPlant) const
{
  m_bendyPipeline->renderBendyPlant(bendyPlant);
}

std::shared_ptr<SmokePipeline> PipelineManager::createSmokeSystem(glm::vec3 position, uint32_t numParticles)
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
  const RenderInfo renderInfo {
    .commandBuffer = commandBuffer,
    .currentFrame = currentFrame,
    .viewPosition = viewPosition,
    .viewMatrix = viewMatrix,
    .extent = extent
  };

  const VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(extent.width),
    .height = static_cast<float>(extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  renderInfo.commandBuffer->setViewport(viewport);

  const VkRect2D scissor = {
    .offset = {0, 0},
    .extent = extent
  };
  renderInfo.commandBuffer->setScissor(scissor);

  renderRenderObjects(renderInfo);

  if (m_shouldDoDots)
  {
    m_dotsPipeline->render(&renderInfo, nullptr);
  }

  m_linePipeline->render(&renderInfo, m_commandPool, m_lineVerticesToRender);

  m_bendyPipeline->render(&renderInfo);

  renderSmokeSystems(renderInfo);

  if (shouldRenderGrid)
  {
    m_gridPipeline->render(&renderInfo);
  }
}

std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& PipelineManager::getRenderObjectsToRender()
{
  return m_renderObjectsToRender;
}

void PipelineManager::renderShadowPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           const RenderInfo& renderInfo)
{
  for (const auto& [_, renderObjects] : m_renderObjectsToRender)
  {
    m_shadowPipeline->render(
      &renderInfo,
      &renderObjects
    );
  }
}

void PipelineManager::renderPointLightShadowMapPipeline(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                        const RenderInfo& renderInfo,
                                                        const std::shared_ptr<PointLight>& light)
{
  for (const auto& [_, renderObjects] : m_renderObjectsToRender)
  {
    m_pointLightShadowMapPipeline->render(
      &renderInfo,
      &renderObjects,
      light
    );
  }
}

void PipelineManager::createPipelines(VkDescriptorSetLayout objectDescriptorSetLayout)
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

  m_shadowPipeline = std::make_unique<ShadowPipeline>(m_logicalDevice, m_renderPass, objectDescriptorSetLayout);

  m_pointLightShadowMapPipeline = std::make_unique<PointLightShadowMapPipeline>(
    m_logicalDevice, m_renderPass, objectDescriptorSetLayout, m_descriptorPool);
}

void PipelineManager::renderRenderObjects(const RenderInfo& renderInfo) const
{
  for (const auto& [type, objects] : m_renderObjectsToRender)
  {
    if (objects.empty())
    {
      continue;
    }

    if (auto it = m_pipelines.find(type); it != m_pipelines.end())
    {
      if (it->first == PipelineType::objectHighlight)
      {
        continue;
      }

      if (auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(it->second.get()))
      {
        graphicsPipeline->displayGui();
        graphicsPipeline->render(&renderInfo, &objects);
        continue;
      }

      throw std::runtime_error("Pipeline for object type is not a GraphicsPipeline");
    }

    throw std::runtime_error("Pipeline for object type does not exist");
  }

  auto highlightObjectsIt = m_renderObjectsToRender.find(PipelineType::objectHighlight);
  auto highlightPipelineIt = m_pipelines.find(PipelineType::objectHighlight);

  if (highlightObjectsIt != m_renderObjectsToRender.end() &&
      highlightPipelineIt != m_pipelines.end())
  {
    auto& highlightObjects = highlightObjectsIt->second;

    if (!highlightObjects.empty())
    {
      if (auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(highlightPipelineIt->second.get()))
      {
        graphicsPipeline->displayGui();
        graphicsPipeline->render(&renderInfo, &highlightObjects);
      }
    }
  }
}

void PipelineManager::renderSmokeSystems(const RenderInfo& renderInfo) const
{
  if (!m_smokeSystems.empty())
  {
    ImGui::Begin("Smoke");
    ImGui::Separator();
    for (const auto& system : m_smokeSystems)
    {
      ImGui::PushID(&system);
      system->displayGui();
      ImGui::PopID();

      ImGui::Separator();

      system->render(&renderInfo, nullptr);
    }
    ImGui::End();
  }
}

} // namespace vke
