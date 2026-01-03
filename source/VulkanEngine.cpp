#include "VulkanEngine.h"
#include "components/assets/AssetManager.h"
#include "components/camera/Camera.h"
#include "components/computingManager/ComputingManager.h"
#include "components/imGui/ImGuiInstance.h"
#include "components/instance/Instance.h"
#include "components/lighting/LightingManager.h"
#include "components/logicalDevice/LogicalDevice.h"
#include "components/physicalDevice/PhysicalDevice.h"
#include "components/pipelines/pipelineManager/PipelineManager.h"
#include "components/renderingManager/Renderer.h"
#include "components/renderingManager/RenderingManager.h"
#include "components/renderingManager/renderer3D/Renderer3D.h"
#include "components/window/Surface.h"
#include "components/window/Window.h"

namespace vke {

  VulkanEngine::VulkanEngine(const EngineConfig& engineConfig)
  {
    glfwInit();

    initializeVulkanAndWindow(engineConfig);

    createPools();

    createComponents(engineConfig);

    createCamera(engineConfig);
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
      m_renderingManager->getRenderer3D()->setCameraParameters(m_camera->getPosition(), m_camera->getViewMatrix());
    }

    m_computingManager->doComputing(m_pipelineManager, m_currentFrame, m_renderingManager->getRenderer2D(),
                                    m_renderingManager->getRenderer3D());

    m_renderingManager->doRendering(m_pipelineManager, m_lightingManager, m_currentFrame);

    createNewFrame();
  }

  std::shared_ptr<AssetManager> VulkanEngine::getAssetManager() const
  {
    return m_assetManager;
  }

  std::shared_ptr<Camera> VulkanEngine::getCamera() const
  {
    return m_camera;
  }

  std::shared_ptr<ImGuiInstance> VulkanEngine::getImGuiInstance() const
  {
    return m_imGuiInstance;
  }

  std::shared_ptr<LightingManager> VulkanEngine::getLightingManager() const
  {
    return m_lightingManager;
  }

  std::shared_ptr<RenderingManager> VulkanEngine::getRenderingManager() const
  {
    return m_renderingManager;
  }

  std::shared_ptr<Window> VulkanEngine::getWindow() const
  {
    return m_window;
  }

  void VulkanEngine::initializeVulkanAndWindow(const EngineConfig& engineConfig)
  {
    m_instance = std::make_shared<Instance>();

    m_window = std::make_shared<Window>(engineConfig.window.width, engineConfig.window.height,
                                        engineConfig.window.title.c_str(), engineConfig.window.fullscreen,
                                        engineConfig.window.resizable);

    m_surface = std::make_shared<Surface>(m_instance, m_window);

    m_physicalDevice = std::make_shared<PhysicalDevice>(m_instance, m_surface);

    m_logicalDevice = std::make_shared<LogicalDevice>(m_physicalDevice);
  }

  void VulkanEngine::createPools()
  {
    createCommandPool();

    createDescriptorPool();
  }

  void VulkanEngine::createComponents(const EngineConfig& engineConfig)
  {
    m_assetManager = std::make_shared<AssetManager>(m_logicalDevice, m_commandPool, m_descriptorPool);

    m_renderingManager = std::make_shared<RenderingManager>(m_logicalDevice, m_surface, m_window,
                                                            m_commandPool,engineConfig.imGui.useDockspace,
                                                            engineConfig.imGui.sceneViewName.c_str(),
                                                            m_assetManager);

    m_lightingManager = std::make_shared<LightingManager>(m_logicalDevice, m_descriptorPool, m_commandPool,
                                                          m_renderingManager->getRenderer());

    m_pipelineManager = std::make_shared<PipelineManager>(m_logicalDevice, m_renderingManager->getRenderer(),
                                                          m_lightingManager, m_assetManager,
                                                          m_descriptorPool, m_commandPool);

    m_imGuiInstance = std::make_shared<ImGuiInstance>(m_window, m_instance, m_logicalDevice,
                                                      m_renderingManager->getRenderer()->getSwapchainRenderPass(),
                                                      engineConfig.imGui.useDockspace, engineConfig.imGui.maxTextures);

    m_computingManager = std::make_shared<ComputingManager>(m_logicalDevice, m_commandPool);
  }

  void VulkanEngine::createCamera(const EngineConfig& engineConfig)
  {
    m_camera = std::make_shared<Camera>(engineConfig.camera.position);
    m_camera->setSpeed(engineConfig.camera.speed);
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

  void VulkanEngine::createNewFrame()
  {
    m_currentFrame = (m_currentFrame + 1) % m_logicalDevice->getMaxFramesInFlight();

    m_imGuiInstance->createNewFrame();

    m_lightingManager->clearLightsToRender();

    m_renderingManager->createNewFrame();
  }

} // namespace vke