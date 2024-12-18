#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include <functional>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include "components/Camera.h"
#include "components/DebugMessenger.h"
#include "components/Framebuffer.h"
#include "components/ImGuiInstance.h"
#include "components/Instance.h"
#include "components/LogicalDevice.h"
#include "components/PhysicalDevice.h"
#include "components/SwapChain.h"
#include "components/Window.h"

#include "pipelines/RenderPass.h"
#include "pipelines/objects/ObjectsPipeline.h"
#include "pipelines/gui/GuiPipeline.h"
#include "pipelines/dots/DotsPipeline.h"

#include "objects/Texture.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "objects/Light.h"

#include "VulkanEngineOptions.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path);
  std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture>& texture,
                                                               const std::shared_ptr<Texture>& specularMap,
                                                               const std::shared_ptr<Model>&) const;

  std::shared_ptr<Light> createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular = 1.0f);

  static ImGuiContext* getImGuiContext();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool sceneIsFocused() const;

private:
  std::unique_ptr<Instance> instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
  std::shared_ptr<Window> window;
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<SwapChain> swapChain;
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<RenderPass> offscreenRenderPass;
  std::unique_ptr<ObjectsPipeline> objectsPipeline;
  std::unique_ptr<GuiPipeline> guiPipeline;
  std::unique_ptr<DotsPipeline> dotsPipeline;

  std::unique_ptr<ImGuiInstance> imGuiInstance;

  std::shared_ptr<Framebuffer> framebuffer;
  std::shared_ptr<Framebuffer> offscreenFramebuffer;

  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;
  std::vector<std::shared_ptr<Light>> lights;

  std::shared_ptr<Camera> camera;

  VulkanEngineOptions vulkanEngineOptions;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> offscreenCommandBuffers;
  std::vector<VkCommandBuffer> swapchainCommandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  uint32_t currentFrame;

  bool framebufferResized;

  bool isSceneFocused;

  VkExtent2D offscreenViewportExtent;

  void initVulkan();
  void createCommandPool();
  void allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const;
  static void recordCommandBuffer(const VkCommandBuffer &commandBuffer, uint32_t imageIndex,
                                  const std::function<void(const VkCommandBuffer &cmdBuffer, uint32_t imgIndex)>& renderFunction);
  void recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const;
  void recordOffscreenCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void recordSwapchainCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void doComputing() const;
  void doRendering();
  void recreateSwapChain();
  void renderGuiScene(uint32_t imageIndex);

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);
};


#endif //VULKANPROJECT_VULKANENGINE_H
