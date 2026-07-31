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
#include "components/renderingManager/FrameScheduler.h"
#include "components/renderingManager/RenderingManager.h"
#include "components/renderingManager/renderer3D/Renderer3D.h"
#include "components/window/Surface.h"
#include "components/window/Window.h"
#include <stdexcept>

namespace vke {

  VulkanEngine::VulkanEngine(const EngineConfig& engineConfig)
  {
    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW!");
    }

    initializeVulkanAndWindow(engineConfig);

    createComponents(engineConfig);

    createCamera(engineConfig);
  }

  VulkanEngine::~VulkanEngine()
  {
    if (m_logicalDevice)
    {
      m_logicalDevice->waitIdle();

      m_renderingManager.reset();
      m_pipelineManager.reset();
      m_surface.reset();
      m_window.reset();

      m_computingManager.reset();
      m_lightingManager.reset();
      m_assetManager.reset();
      m_imGuiInstance.reset();

      m_logicalDevice.reset();
      m_physicalDevice.reset();
      m_instance.reset();
    }

    glfwTerminate();
  }

  bool VulkanEngine::isActive() const
  {
    if (m_window)
    {
      return m_window->isOpen();
    }

    return false;
  }

  void VulkanEngine::render()
  {
    m_window->update();

    if (m_renderingManager->isSceneFocused() && m_camera->isEnabled())
    {
      m_camera->processInput(m_window);
      m_renderingManager->getRenderer3D()->setCameraParameters(m_camera->getPosition(), m_camera->getViewMatrix());
    }

    const auto frameScheduler = m_renderingManager->getFrameScheduler();

    frameScheduler->beginFrame();

    const uint32_t currentFrame = frameScheduler->getCurrentFrame();

    m_computingManager->doComputing(m_pipelineManager, currentFrame, m_renderingManager->getRenderer2D(),
                                    m_renderingManager->getRenderer3D());

    m_renderingManager->doRendering(m_pipelineManager, m_lightingManager, currentFrame);

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

  std::shared_ptr<PhysicalDevice> VulkanEngine::getPhysicalDevice() const
  {
    return m_physicalDevice;
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

    m_window = std::make_shared<Window>(engineConfig.window);

    m_surface = std::make_shared<Surface>(m_instance, m_window);

    m_physicalDevice = std::make_shared<PhysicalDevice>(m_instance, m_surface, engineConfig.device);

    m_logicalDevice = std::make_shared<LogicalDevice>(m_physicalDevice);
  }

  void VulkanEngine::createComponents(const EngineConfig& engineConfig)
  {
    m_assetManager = std::make_shared<AssetManager>(m_logicalDevice);

    m_renderingManager = std::make_shared<RenderingManager>(
      m_logicalDevice,
      m_surface,
      m_window,
      engineConfig.imGui.sceneViewName.c_str(),
      engineConfig.imGui.useDockspace,
      engineConfig.rendering.rayTracingEnabled,
      m_assetManager
    );

    m_lightingManager = std::make_shared<LightingManager>(m_logicalDevice);

    m_pipelineManager = std::make_shared<PipelineManager>(
      m_logicalDevice,
      m_renderingManager,
      m_lightingManager,
      m_assetManager
    );

    m_imGuiInstance = std::make_shared<ImGuiInstance>(
      m_window,
      m_instance,
      m_logicalDevice,
      m_renderingManager->getSwapChainImageFormat(),
      engineConfig.imGui
    );

    m_computingManager = std::make_shared<ComputingManager>(m_logicalDevice, m_renderingManager->getFrameScheduler());
  }

  void VulkanEngine::createCamera(const EngineConfig& engineConfig)
  {
    m_camera = std::make_shared<Camera>(engineConfig.camera);
  }

  void VulkanEngine::createNewFrame()
  {
    m_imGuiInstance->createNewFrame();

    m_lightingManager->clearLightsToRender();

    m_renderingManager->createNewFrame();
  }

} // namespace vke