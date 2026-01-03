#ifndef VKE_VULKANENGINE_H
#define VKE_VULKANENGINE_H

#include "EngineConfig.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class AssetManager;
  class Camera;
  class ComputingManager;
  class ImGuiInstance;
  class Instance;
  class LightingManager;
  class LogicalDevice;
  class PhysicalDevice;
  class PipelineManager;
  class RenderingManager;
  class Surface;
  class Window;

  class VulkanEngine {
  public:
    explicit VulkanEngine(const EngineConfig& engineConfig);

    ~VulkanEngine();

    [[nodiscard]] bool isActive() const;

    void render();

    [[nodiscard]] std::shared_ptr<AssetManager> getAssetManager() const;

    [[nodiscard]] std::shared_ptr<Camera> getCamera() const;

    [[nodiscard]] std::shared_ptr<ImGuiInstance> getImGuiInstance() const;

    [[nodiscard]] std::shared_ptr<LightingManager> getLightingManager() const;

    [[nodiscard]] std::shared_ptr<RenderingManager> getRenderingManager() const;

    [[nodiscard]] std::shared_ptr<Window> getWindow() const;

  private:
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Surface> m_surface;
    std::shared_ptr<Window> m_window;
    std::shared_ptr<PhysicalDevice> m_physicalDevice;
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<ImGuiInstance> m_imGuiInstance;

    std::shared_ptr<LightingManager> m_lightingManager;

    std::shared_ptr<PipelineManager> m_pipelineManager;

    std::shared_ptr<RenderingManager> m_renderingManager;

    std::shared_ptr<ComputingManager> m_computingManager;

    std::shared_ptr<AssetManager> m_assetManager;

    std::shared_ptr<Camera> m_camera;

    uint32_t m_currentFrame = 0;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    void initializeVulkanAndWindow(const EngineConfig& engineConfig);

    void createPools();

    void createComponents(const EngineConfig& engineConfig);

    void createCamera(const EngineConfig& engineConfig);

    void createCommandPool();

    void createDescriptorPool();

    void createNewFrame();
  };

} // namespace vke

#endif //VKE_VULKANENGINE_H
