#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

#include "components/Instance.h"
#include "components/DebugMessenger.h"
#include "components/Window.h"
#include "components/PhysicalDevice.h"
#include "components/LogicalDevice.h"
#include "components/SwapChain.h"
#include "components/ImGuiInstance.h"
#include "components/Camera.h"
#include "components/Framebuffer.h"

#include "pipelines/RenderPass.h"
#include "pipelines/objects/ObjectsPipeline.h"
#include "pipelines/gui/GuiPipeline.h"
#include "pipelines/compute/ComputePipeline.h"

#include "objects/Texture.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"

#include "VulkanEngineOptions.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path);
  std::shared_ptr<Model> loadModel(const char* path);
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture>& texture, const std::shared_ptr<Texture>& specularMap,
                                                 const std::shared_ptr<Model>&) const;

private:
  void initVulkan();
  void createCommandPool();
  void createCommandBuffers();
  void createComputeCommandBuffers();
  void recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const;
  void recordCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void doComputing() const;
  void doRendering();
  void recreateSwapChain();

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
  std::unique_ptr<Instance> instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
  std::shared_ptr<Window> window;
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<SwapChain> swapChain;
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<Framebuffer> framebuffer;
  std::unique_ptr<ObjectsPipeline> objectsPipeline;
  std::unique_ptr<GuiPipeline> guiPipeline;
  std::unique_ptr<ComputePipeline> computePipeline;

  std::unique_ptr<ImGuiInstance> imGuiInstance;
  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;

  std::shared_ptr<Camera> camera;

  VulkanEngineOptions vulkanEngineOptions;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  uint32_t currentFrame;

  bool framebufferResized;
};


#endif //VULKANPROJECT_VULKANENGINE_H
