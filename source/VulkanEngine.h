#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include "components/window/Window.h"
#include "VulkanEngineOptions.h"
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class Camera;
class CommandBuffer;
class ImGuiInstance;
class LightingManager;
class LogicalDevice;
class Model;
class MousePicker;
class PhysicalDevice;
class PipelineManager;
class RenderObject;
struct RenderInfo;
class RenderingManager;
class Texture;
class Texture2D;

class VulkanEngine {
public:
  explicit VulkanEngine(const VulkanEngineOptions& vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture2D> loadTexture(const char* path, bool repeat = true);
  std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                               const std::shared_ptr<Texture2D>& specularMap,
                                                               const std::shared_ptr<Model>&);

  [[nodiscard]] std::shared_ptr<LightingManager> getLightingManager() const;

  [[nodiscard]] std::shared_ptr<MousePicker> getMousePicker() const;

  [[nodiscard]] std::shared_ptr<Window> getWindow() const;

  [[nodiscard]] std::shared_ptr<PipelineManager> getPipelineManager() const;

  [[nodiscard]] std::shared_ptr<RenderingManager> getRenderingManager() const;

  [[nodiscard]] std::shared_ptr<ImGuiInstance> getImGuiInstance() const;

private:
  VulkanEngineOptions m_vulkanEngineOptions;

  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<PhysicalDevice> m_physicalDevice;
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<LightingManager> m_lightingManager;

  std::shared_ptr<RenderingManager> m_renderingManager;

  std::shared_ptr<PipelineManager> m_pipelineManager;

  std::shared_ptr<ImGuiInstance> m_imGuiInstance;

  std::shared_ptr<MousePicker> m_mousePicker;

  std::shared_ptr<Camera> m_camera;

  std::vector<std::shared_ptr<Texture>> m_textures;
  std::vector<std::shared_ptr<Model>> m_models;
  std::vector<std::shared_ptr<RenderObject>> m_renderObjects;

  uint32_t m_currentFrame;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  std::shared_ptr<CommandBuffer> m_computeCommandBuffer;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  void initVulkan();
  void createCommandPool();

  void recordComputeCommandBuffer() const;

  void doComputing() const;

  void createNewFrame() const;

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void createDescriptorPool();

  void createObjectDescriptorSetLayout();
};


#endif //VULKANPROJECT_VULKANENGINE_H
