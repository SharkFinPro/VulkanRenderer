#include "VulkanEngine.h"

#include "components/framebuffers/StandardFramebuffer.h"
#include "components/framebuffers/SwapchainFramebuffer.h"
#include "components/textures/Texture2D.h"
#include "components/Camera.h"
#include "components/ImGuiInstance.h"
#include "components/lighting/LightingManager.h"
#include "components/MousePicker.h"
#include "components/window/SwapChain.h"

#include "components/core/commandBuffer/CommandBuffer.h"
#include "components/core/instance/Instance.h"
#include "components/core/logicalDevice/LogicalDevice.h"
#include "components/core/physicalDevice/PhysicalDevice.h"

#include "components/objects/Model.h"
#include "components/objects/RenderObject.h"

#include "pipelines/custom/renderObject/BumpyCurtain.h"
#include "pipelines/custom/renderObject/CrossesPipeline.h"
#include "pipelines/custom/renderObject/CubeMapPipeline.h"
#include "pipelines/custom/renderObject/CurtainPipeline.h"
#include "pipelines/custom/renderObject/EllipticalDots.h"
#include "pipelines/custom/renderObject/MagnifyWhirlMosaicPipeline.h"
#include "pipelines/custom/renderObject/NoisyEllipticalDots.h"
#include "pipelines/custom/renderObject/ObjectHighlightPipeline.h"
#include "pipelines/custom/renderObject/ObjectsPipeline.h"
#include "pipelines/custom/renderObject/SnakePipeline.h"
#include "pipelines/custom/renderObject/TexturedPlane.h"

#include "pipelines/custom/BendyPipeline.h"
#include "pipelines/custom/DotsPipeline.h"
#include "pipelines/custom/GuiPipeline.h"
#include "pipelines/custom/LinePipeline.h"
#include "pipelines/custom/SmokePipeline.h"

#include "pipelines/RenderPass.h"

#include <ranges>
#include <stdexcept>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VulkanEngine::VulkanEngine(const VulkanEngineOptions& vulkanEngineOptions)
  : m_vulkanEngineOptions(vulkanEngineOptions), m_currentFrame(0), m_framebufferResized(false), m_isSceneFocused(false),
    m_useCamera(true)
{
  glfwInit();
  initVulkan();

  m_camera = std::make_shared<Camera>(m_vulkanEngineOptions.CAMERA_POSITION);
  m_camera->setSpeed(m_vulkanEngineOptions.CAMERA_SPEED);
}

VulkanEngine::~VulkanEngine()
{
  m_logicalDevice->waitIdle();

  m_logicalDevice->destroyDescriptorPool(m_descriptorPool);

  m_logicalDevice->destroyDescriptorSetLayout(m_objectDescriptorSetLayout);

  m_logicalDevice->destroyCommandPool(m_commandPool);

  glfwTerminate();
}

bool VulkanEngine::isActive() const
{
  return m_window->isOpen();
}

void VulkanEngine::render()
{
  m_window->update();

  if (sceneIsFocused() && m_useCamera)
  {
    m_camera->processInput(m_window);
    setCameraParameters(m_camera->getPosition(), m_camera->getViewMatrix());
  }

  doComputing();

  doRendering();

  createNewFrame();
}

std::shared_ptr<Texture2D> VulkanEngine::loadTexture(const char* path, const bool repeat)
{
  auto texture = std::make_shared<Texture2D>(m_logicalDevice, m_commandPool, path,
                                             repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  m_textures.push_back(texture);

  return texture;
}

std::shared_ptr<Model> VulkanEngine::loadModel(const char* path, glm::vec3 rotation)
{
  auto model = std::make_shared<Model>(
    m_logicalDevice,
    m_commandPool,
    path,
    rotation
  );

  m_models.push_back(model);

  return model;
}

std::shared_ptr<RenderObject> VulkanEngine::loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                             const std::shared_ptr<Texture2D>& specularMap,
                                                             const std::shared_ptr<Model>& model)
{
  auto renderObject = std::make_shared<RenderObject>(
    m_logicalDevice,
    m_objectDescriptorSetLayout,
    texture,
    specularMap,
    model
  );

  m_renderObjects.push_back(renderObject);

  return renderObject;
}

std::shared_ptr<Light> VulkanEngine::createLight(const glm::vec3 position, const glm::vec3 color, const float ambient,
                                                 const float diffuse, const float specular) const
{
  return m_lightingManager->createLight(position, color, ambient, diffuse, specular);
}

ImGuiContext* VulkanEngine::getImGuiContext()
{
  return ImGui::GetCurrentContext();
}

bool VulkanEngine::keyIsPressed(const int key) const
{
  return m_window->keyIsPressed(key);
}

bool VulkanEngine::buttonIsPressed(const int button) const
{
  return m_window->buttonDown(button);
}

bool VulkanEngine::sceneIsFocused() const
{
  return m_isSceneFocused || !m_vulkanEngineOptions.USE_DOCKSPACE;
}

void VulkanEngine::renderObject(const std::shared_ptr<RenderObject>& renderObject, const PipelineType pipelineType,
                                bool* mousePicked)
{
  m_renderObjectsToRender[pipelineType].push_back(renderObject);

  if (mousePicked == nullptr)
  {
    return;
  }

  m_mousePicker->renderObject(renderObject, mousePicked);
}

void VulkanEngine::renderLight(const std::shared_ptr<Light>& light) const
{
  m_lightingManager->renderLight(light);
}

void VulkanEngine::renderLine(const glm::vec3 start, const glm::vec3 end)
{
  m_lineVerticesToRender.push_back({start});
  m_lineVerticesToRender.push_back({end});
}

void VulkanEngine::renderBendyPlant(const BendyPlant& bendyPlant) const
{
  m_bendyPipeline->renderBendyPlant(bendyPlant);
}

void VulkanEngine::enableCamera()
{
  m_useCamera = true;
}

void VulkanEngine::disableCamera()
{
  m_useCamera = false;
}

void VulkanEngine::setCameraParameters(const glm::vec3 position, const glm::mat4& viewMatrix)
{
  m_viewPosition = position;
  m_viewMatrix = viewMatrix;
}

std::shared_ptr<ImGuiInstance> VulkanEngine::getImGuiInstance() const
{
  return m_imGuiInstance;
}

std::shared_ptr<SmokePipeline> VulkanEngine::createSmokeSystem(const glm::vec3 position, const uint32_t numParticles)
{
  auto system = std::make_shared<SmokePipeline>(m_logicalDevice, m_commandPool, m_renderPass->getRenderPass(),
                                                m_descriptorPool, position, numParticles,
                                                m_lightingManager->getLightingDescriptorSet());

  m_smokeSystems.push_back(system);

  return system;
}

void VulkanEngine::destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem)
{
  const auto system = std::ranges::find(m_smokeSystems, smokeSystem);

  if (system == m_smokeSystems.end())
  {
    return;
  }

  m_logicalDevice->waitIdle();
  m_smokeSystems.erase(system);
}

bool VulkanEngine::canMousePick() const
{
  return m_mousePicker->canMousePick();
}

void VulkanEngine::initVulkan()
{
  m_instance = std::make_shared<Instance>();

  m_window = std::make_shared<Window>(m_vulkanEngineOptions.WINDOW_WIDTH, m_vulkanEngineOptions.WINDOW_HEIGHT,
                                      m_vulkanEngineOptions.WINDOW_TITLE, m_instance,
                                      m_vulkanEngineOptions.FULLSCREEN);

  m_physicalDevice = std::make_shared<PhysicalDevice>(m_instance, m_window->getSurface());

  m_logicalDevice = std::make_shared<LogicalDevice>(m_physicalDevice);

  createCommandPool();

  m_computeCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
  m_offscreenCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
  m_swapchainCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

  createDescriptorPool();

  m_lightingManager = std::make_unique<LightingManager>(m_logicalDevice, m_descriptorPool);

  createObjectDescriptorSetLayout();

  m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window);

  m_renderPass = std::make_shared<RenderPass>(m_logicalDevice, m_swapChain->getImageFormat(),
                                              m_physicalDevice->getMsaaSamples(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  m_offscreenRenderPass = std::make_shared<RenderPass>(m_logicalDevice, VK_FORMAT_B8G8R8A8_UNORM,
                                                       m_physicalDevice->getMsaaSamples(),
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  m_pipelines[PipelineType::object] = std::make_unique<ObjectsPipeline>(
    m_logicalDevice, m_renderPass, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::objectHighlight] = std::make_unique<ObjectHighlightPipeline>(
    m_logicalDevice, m_renderPass, m_objectDescriptorSetLayout);

  m_pipelines[PipelineType::ellipticalDots] = std::make_unique<EllipticalDots>(
    m_logicalDevice, m_renderPass, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::noisyEllipticalDots] = std::make_unique<NoisyEllipticalDots>(
    m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::bumpyCurtain] = std::make_unique<BumpyCurtain>(
    m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::curtain] = std::make_unique<CurtainPipeline>(
    m_logicalDevice, m_renderPass, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::cubeMap] = std::make_unique<CubeMapPipeline>(
    m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool, m_objectDescriptorSetLayout);

  m_pipelines[PipelineType::texturedPlane] = std::make_unique<TexturedPlane>(
    m_logicalDevice, m_renderPass, m_objectDescriptorSetLayout);

  m_pipelines[PipelineType::magnifyWhirlMosaic] = std::make_unique<MagnifyWhirlMosaicPipeline>(
    m_logicalDevice, m_renderPass, m_descriptorPool, m_objectDescriptorSetLayout);

  m_pipelines[PipelineType::snake] = std::make_unique<SnakePipeline>(
    m_logicalDevice, m_renderPass, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_pipelines[PipelineType::crosses] = std::make_unique<CrossesPipeline>(
    m_logicalDevice, m_renderPass, m_descriptorPool, m_objectDescriptorSetLayout,
    m_lightingManager->getLightingDescriptorSet());

  m_guiPipeline = std::make_unique<GuiPipeline>(m_logicalDevice, m_renderPass,
                                                m_vulkanEngineOptions.MAX_IMGUI_TEXTURES);

  if (m_vulkanEngineOptions.DO_DOTS)
  {
    m_dotsPipeline = std::make_unique<DotsPipeline>(m_logicalDevice, m_commandPool,
                                                    m_renderPass->getRenderPass(), m_swapChain->getExtent(), m_descriptorPool);
  }

  m_linePipeline = std::make_unique<LinePipeline>(m_logicalDevice, m_renderPass, m_descriptorPool);

  m_imGuiInstance = std::make_shared<ImGuiInstance>(m_window, m_instance, m_logicalDevice, m_renderPass, m_guiPipeline,
                                                    m_vulkanEngineOptions.USE_DOCKSPACE);

  m_framebuffer = std::make_shared<SwapchainFramebuffer>(m_logicalDevice, m_swapChain, m_commandPool, m_renderPass,
                                                         m_swapChain->getExtent());

  if (m_vulkanEngineOptions.USE_DOCKSPACE)
  {
    m_offscreenFramebuffer = std::make_shared<StandardFramebuffer>(m_logicalDevice, m_commandPool, m_renderPass,
                                                                   m_swapChain->getExtent());
  }

  m_mousePicker = std::make_unique<MousePicker>(m_logicalDevice, m_window, m_commandPool, m_objectDescriptorSetLayout);
  if (!m_vulkanEngineOptions.USE_DOCKSPACE)
  {
    m_mousePicker->recreateFramebuffer(m_swapChain->getExtent());
  }

  m_bendyPipeline = std::make_unique<BendyPipeline>(m_logicalDevice, m_renderPass, m_commandPool, m_descriptorPool);
}

void VulkanEngine::createCommandPool()
{
  const auto queueFamilyIndices = m_physicalDevice->getQueueFamilies();

  const VkCommandPoolCreateInfo poolInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
  };

  m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
}

void VulkanEngine::recordComputeCommandBuffer() const
{
  m_computeCommandBuffer->record([this]()
  {
    if (m_vulkanEngineOptions.DO_DOTS)
    {
      m_dotsPipeline->compute(m_computeCommandBuffer, m_currentFrame);
    }

    for (const auto& system : m_smokeSystems)
    {
      system->compute(m_computeCommandBuffer, m_currentFrame);
    }
  });
}

void VulkanEngine::recordOffscreenCommandBuffer(const uint32_t imageIndex) const
{
  m_offscreenCommandBuffer->record([this, imageIndex]()
  {
    if (!m_vulkanEngineOptions.USE_DOCKSPACE ||
        m_offscreenViewportExtent.width == 0 ||
        m_offscreenViewportExtent.height == 0)
    {
      return;
    }

    m_offscreenRenderPass->begin(m_offscreenFramebuffer->getFramebuffer(imageIndex), m_offscreenViewportExtent, m_offscreenCommandBuffer);

    renderGraphicsPipelines(m_offscreenCommandBuffer, m_offscreenViewportExtent);

    m_offscreenCommandBuffer->endRenderPass();
  });
}

void VulkanEngine::recordSwapchainCommandBuffer(const uint32_t imageIndex) const
{
  m_swapchainCommandBuffer->record([this, imageIndex]()
  {
    const RenderInfo renderInfo {
      .commandBuffer = m_swapchainCommandBuffer,
      .currentFrame = m_currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = m_swapChain->getExtent()
    };

    m_renderPass->begin(m_framebuffer->getFramebuffer(imageIndex), m_swapChain->getExtent(), renderInfo.commandBuffer);

    if (!m_vulkanEngineOptions.USE_DOCKSPACE)
    {
      renderGraphicsPipelines(renderInfo.commandBuffer, m_swapChain->getExtent());
    }

    const VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(renderInfo.extent.width),
      .height = static_cast<float>(renderInfo.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    renderInfo.commandBuffer->setViewport(viewport);

    const VkRect2D scissor = {
      .offset = {0, 0},
      .extent = renderInfo.extent
    };
    renderInfo.commandBuffer->setScissor(scissor);

    m_guiPipeline->render(&renderInfo);

    renderInfo.commandBuffer->endRenderPass();
  });
}

void VulkanEngine::doComputing() const
{
  m_logicalDevice->waitForComputeFences(m_currentFrame);

  m_logicalDevice->resetComputeFences(m_currentFrame);

  m_computeCommandBuffer->setCurrentFrame(m_currentFrame);
  m_computeCommandBuffer->resetCommandBuffer();
  recordComputeCommandBuffer();

  m_logicalDevice->submitComputeQueue(m_currentFrame, m_computeCommandBuffer->getCommandBuffer());
}

void VulkanEngine::doRendering()
{
  m_logicalDevice->waitForGraphicsFences(m_currentFrame);

  uint32_t imageIndex;
  auto result = m_logicalDevice->acquireNextImage(m_currentFrame, m_swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m_framebufferResized = false;
    recreateSwapChain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  renderGuiScene(imageIndex);

  m_logicalDevice->resetGraphicsFences(m_currentFrame);

  m_lightingManager->update(m_currentFrame, m_camera->getPosition());

  m_mousePicker->doMousePicking(imageIndex, m_currentFrame, m_viewPosition, m_viewMatrix, m_renderObjectsToRender);

  m_offscreenCommandBuffer->setCurrentFrame(m_currentFrame);
  m_offscreenCommandBuffer->resetCommandBuffer();
  recordOffscreenCommandBuffer(imageIndex);
  m_logicalDevice->submitOffscreenGraphicsQueue(m_currentFrame, m_offscreenCommandBuffer->getCommandBuffer());

  m_swapchainCommandBuffer->setCurrentFrame(m_currentFrame);
  m_swapchainCommandBuffer->resetCommandBuffer();
  recordSwapchainCommandBuffer(imageIndex);
  m_logicalDevice->submitGraphicsQueue(m_currentFrame, m_swapchainCommandBuffer->getCommandBuffer());

  result = m_logicalDevice->queuePresent(m_currentFrame, m_swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
  {
    m_framebufferResized = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  m_currentFrame = (m_currentFrame + 1) % m_logicalDevice->getMaxFramesInFlight();
}

void VulkanEngine::recreateSwapChain()
{
  int width = 0, height = 0;
  m_window->getFramebufferSize(&width, &height);
  while (width == 0 || height == 0)
  {
    m_window->getFramebufferSize(&width, &height);
    glfwWaitEvents();
  }

  m_logicalDevice->waitIdle();

  m_framebuffer.reset();
  m_swapChain.reset();

  m_physicalDevice->updateSwapChainSupportDetails();

  m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window);
  m_framebuffer = std::make_shared<SwapchainFramebuffer>(m_logicalDevice, m_swapChain, m_commandPool, m_renderPass,
                                                         m_swapChain->getExtent());

  if (!m_vulkanEngineOptions.USE_DOCKSPACE)
  {
    m_mousePicker->recreateFramebuffer(m_swapChain->getExtent());
  }


  if (m_vulkanEngineOptions.USE_DOCKSPACE)
  {
    if (m_offscreenViewportExtent.width == 0 || m_offscreenViewportExtent.height == 0)
    {
      return;
    }

    m_offscreenFramebuffer.reset();

    m_offscreenFramebuffer = std::make_shared<StandardFramebuffer>(m_logicalDevice, m_commandPool, m_renderPass,
                                                                   m_offscreenViewportExtent);

    m_mousePicker->recreateFramebuffer(m_offscreenViewportExtent);
  }
}

void VulkanEngine::renderGuiScene(const uint32_t imageIndex)
{
  if (!m_vulkanEngineOptions.USE_DOCKSPACE)
  {
    return;
  }

  ImGui::Begin(m_vulkanEngineOptions.SCENE_VIEW_NAME);

  m_isSceneFocused = ImGui::IsWindowFocused();

  const auto contentRegionAvailable = ImGui::GetContentRegionAvail();

  const VkExtent2D currentOffscreenViewportExtent {
    .width = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.x)),
    .height = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.y))
  };

  if (currentOffscreenViewportExtent.width == 0 || currentOffscreenViewportExtent.height == 0)
  {
    m_offscreenViewportExtent = currentOffscreenViewportExtent;
    ImGui::End();
    return;
  }

  if (m_offscreenViewportExtent.width != currentOffscreenViewportExtent.width ||
      m_offscreenViewportExtent.height != currentOffscreenViewportExtent.height)
  {
    m_offscreenViewportExtent = currentOffscreenViewportExtent;

    m_logicalDevice->waitIdle();
    m_offscreenFramebuffer.reset();
    m_offscreenFramebuffer = std::make_shared<StandardFramebuffer>(m_logicalDevice, m_commandPool, m_renderPass,
                                                                   m_offscreenViewportExtent);

    m_mousePicker->recreateFramebuffer(m_offscreenViewportExtent);
  }

  m_offscreenViewportPos = ImGui::GetCursorScreenPos();
  m_mousePicker->setViewportPos(m_offscreenViewportPos);

  ImGui::Image(reinterpret_cast<ImTextureID>(m_offscreenFramebuffer->getFramebufferImageDescriptorSet(imageIndex)),
              contentRegionAvailable);

  ImGui::End();
}

void VulkanEngine::renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer, const VkExtent2D extent) const
{
  const RenderInfo renderInfo {
    .commandBuffer = commandBuffer,
    .currentFrame = m_currentFrame,
    .viewPosition = m_viewPosition,
    .viewMatrix = m_viewMatrix,
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

  if (m_vulkanEngineOptions.DO_DOTS)
  {
    m_dotsPipeline->render(&renderInfo, nullptr);
  }

  m_linePipeline->render(&renderInfo, m_commandPool, m_lineVerticesToRender);

  m_bendyPipeline->render(&renderInfo);

  renderSmokeSystems(renderInfo);
}

void VulkanEngine::renderRenderObjects(const RenderInfo& renderInfo) const
{
  for (const auto& [type, objects] : m_renderObjectsToRender)
  {
    if (objects.empty())
    {
      continue;
    }

    if (auto it = m_pipelines.find(type); it != m_pipelines.end())
    {
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
}

void VulkanEngine::renderSmokeSystems(const RenderInfo& renderInfo) const
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

void VulkanEngine::createNewFrame()
{
  m_imGuiInstance->createNewFrame();

  for (auto& [_, objects] : m_renderObjectsToRender)
  {
    objects.clear();
  }

  m_lightingManager->clearLightsToRender();

  m_lineVerticesToRender.clear();

  m_mousePicker->clearObjectsToMousePick();

  m_bendyPipeline->clearBendyPlantsToRender();
}

void VulkanEngine::createDescriptorPool()
{
  const std::array<VkDescriptorPoolSize, 3> poolSizes {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight() * 30},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_logicalDevice->getMaxFramesInFlight() * 50},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_logicalDevice->getMaxFramesInFlight() * 10}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = m_logicalDevice->getMaxFramesInFlight() * 30,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
}

void VulkanEngine::createObjectDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding textureLayout {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding specularLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array objectBindings {
    transformLayout,
    textureLayout,
    specularLayout
  };

  const VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(objectBindings.size()),
    .pBindings = objectBindings.data()
  };

  m_objectDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(objectLayoutCreateInfo);
}
