#include "VulkanEngine.h"
#include "components/AssetManager.h"
#include "components/computingManager/ComputingManager.h"
#include "components/core/instance/Instance.h"
#include "components/core/logicalDevice/LogicalDevice.h"
#include "components/core/physicalDevice/PhysicalDevice.h"
#include "components/lighting/LightingManager.h"
#include "components/renderingManager/Renderer.h"
#include "components/renderingManager/RenderingManager.h"
#include "components/Camera.h"
#include "components/ImGuiInstance.h"
#include "components/MousePicker.h"
#include "components/PipelineManager.h"

VulkanEngine::VulkanEngine(const VulkanEngineOptions& vulkanEngineOptions)
  : m_vulkanEngineOptions(vulkanEngineOptions), m_currentFrame(0)
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

  if (m_renderingManager->isSceneFocused() && m_camera->isEnabled())
  {
    m_camera->processInput(m_window);
    m_renderingManager->setCameraParameters(m_camera->getPosition(), m_camera->getViewMatrix());
  }

  m_computingManager->doComputing(m_pipelineManager, m_currentFrame);

  m_renderingManager->doRendering(m_pipelineManager, m_lightingManager, m_currentFrame);

  createNewFrame();
}

std::shared_ptr<LightingManager> VulkanEngine::getLightingManager() const
{
  return m_lightingManager;
}

std::shared_ptr<MousePicker> VulkanEngine::getMousePicker() const
{
  return m_mousePicker;
}

std::shared_ptr<Window> VulkanEngine::getWindow() const
{
  return m_window;
}

std::shared_ptr<PipelineManager> VulkanEngine::getPipelineManager() const
{
  return m_pipelineManager;
}

std::shared_ptr<RenderingManager> VulkanEngine::getRenderingManager() const
{
  return m_renderingManager;
}

std::shared_ptr<ImGuiInstance> VulkanEngine::getImGuiInstance() const
{
  return m_imGuiInstance;
}

std::shared_ptr<AssetManager> VulkanEngine::getAssetManager() const
{
  return m_assetManager;
}

std::shared_ptr<Camera> VulkanEngine::getCamera() const
{
  return m_camera;
}

void VulkanEngine::initVulkan()
{
  m_instance = std::make_shared<Instance>();

  m_window = std::make_shared<Window>(m_vulkanEngineOptions.WINDOW_WIDTH, m_vulkanEngineOptions.WINDOW_HEIGHT,
                                      m_vulkanEngineOptions.WINDOW_TITLE, m_instance,
                                      m_vulkanEngineOptions.FULLSCREEN, this);

  m_physicalDevice = std::make_shared<PhysicalDevice>(m_instance, m_window->getSurface());

  m_logicalDevice = std::make_shared<LogicalDevice>(m_physicalDevice);

  createCommandPool();

  createDescriptorPool();

  m_lightingManager = std::make_shared<LightingManager>(m_logicalDevice, m_descriptorPool);

  m_assetManager = std::make_shared<AssetManager>(m_logicalDevice, m_commandPool);

  m_mousePicker = std::make_shared<MousePicker>(m_logicalDevice, m_window, m_commandPool,
                                                m_assetManager->getObjectDescriptorSetLayout());

  m_renderingManager = std::make_shared<RenderingManager>(m_logicalDevice, m_window, m_mousePicker, m_commandPool,
                                                          m_vulkanEngineOptions.USE_DOCKSPACE,
                                                          m_vulkanEngineOptions.SCENE_VIEW_NAME);

  m_pipelineManager = std::make_shared<PipelineManager>(m_logicalDevice, m_renderingManager->getRenderer()->getRenderPass(),
                                                        m_lightingManager, m_mousePicker,
                                                        m_assetManager->getObjectDescriptorSetLayout(),
                                                        m_descriptorPool, m_commandPool, m_vulkanEngineOptions.DO_DOTS);

  m_imGuiInstance = std::make_shared<ImGuiInstance>(m_window, m_instance, m_logicalDevice,
                                                    m_renderingManager->getRenderer()->getRenderPass(),
                                                    m_vulkanEngineOptions.USE_DOCKSPACE,
                                                    m_vulkanEngineOptions.MAX_IMGUI_TEXTURES);

  m_computingManager = std::make_shared<ComputingManager>(m_logicalDevice, m_commandPool);
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

void VulkanEngine::createNewFrame() const
{
  m_imGuiInstance->createNewFrame();

  m_lightingManager->clearLightsToRender();

  m_mousePicker->clearObjectsToMousePick();

  m_pipelineManager->createNewFrame();
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
