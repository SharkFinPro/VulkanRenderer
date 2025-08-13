#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include "VulkanEngineOptions.h"
#include <vulkan/vulkan.h>
#include <memory>

class AssetManager;
class Camera;
class ComputingManager;
class ImGuiInstance;
class Instance;
class LightingManager;
class LogicalDevice;
class MousePicker;
class PhysicalDevice;
class PipelineManager;
struct RenderInfo;
class RenderingManager;
class Window;

class VulkanEngine {
public:
  explicit VulkanEngine(const VulkanEngineOptions& vulkanEngineOptions);

  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  [[nodiscard]] std::shared_ptr<AssetManager> getAssetManager() const;

  [[nodiscard]] std::shared_ptr<Camera> getCamera() const;

  [[nodiscard]] std::shared_ptr<ImGuiInstance> getImGuiInstance() const;

  [[nodiscard]] std::shared_ptr<LightingManager> getLightingManager() const;

  [[nodiscard]] std::shared_ptr<MousePicker> getMousePicker() const;

  [[nodiscard]] std::shared_ptr<PipelineManager> getPipelineManager() const;

  [[nodiscard]] std::shared_ptr<RenderingManager> getRenderingManager() const;

  [[nodiscard]] std::shared_ptr<Window> getWindow() const;

private:
  VulkanEngineOptions m_vulkanEngineOptions;

  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<PhysicalDevice> m_physicalDevice;
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<ImGuiInstance> m_imGuiInstance;

  std::shared_ptr<MousePicker> m_mousePicker;

  std::shared_ptr<LightingManager> m_lightingManager;

  std::shared_ptr<PipelineManager> m_pipelineManager;

  std::shared_ptr<RenderingManager> m_renderingManager;

  std::shared_ptr<ComputingManager> m_computingManager;

  std::shared_ptr<AssetManager> m_assetManager;

  std::shared_ptr<Camera> m_camera;

  uint32_t m_currentFrame;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  void initializeVulkanAndWindow();

  void createPools();

  void createManagers();

  void createCamera();

  void createCommandPool();

  void createDescriptorPool();

  void createNewFrame() const;
};


#endif //VULKANPROJECT_VULKANENGINE_H
