#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include <vector>
#include <string>
#include <memory>
#include <vulkan/vulkan.h>

#include "components/Instance.h"
#include "components/DebugMessenger.h"
#include "components/Window.h"
#include "components/PhysicalDevice.h"
#include "components/LogicalDevice.h"
#include "components/Camera.h"
#include "pipeline/RenderPass.h"

#include "pipeline/GraphicsPipeline.h"
#include "pipeline/GuiPipeline.h"
#include "objects/Texture.h"

#include "objects/Model.h"
#include "objects/RenderObject.h"

#include "VulkanEngineOptions.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path);
  std::shared_ptr<Model> loadModel(const char* path);
  std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture>& texture, const std::shared_ptr<Texture>& specularMap,
                                                 const std::shared_ptr<Model>&);

private:
  void initVulkan();
  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
  void createSwapChain();
  void createImageViews();
  void createFrameBuffers();
  void createCommandPool();
  void createCommandBuffers();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void drawFrame();
  void createSyncObjects();
  void cleanupSwapChain();
  void recreateSwapChain();
  void createDepthResources();
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  void createColorResources();
  void initImGui();

  friend class Window;

private:
  std::unique_ptr<Instance> instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
  std::shared_ptr<Window> window;
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::unique_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<RenderPass> renderPass;
  std::unique_ptr<GraphicsPipeline> graphicsPipeline;
  std::unique_ptr<GuiPipeline> guiPipeline;

  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;
  std::shared_ptr<Camera> camera;

  VulkanEngineOptions vulkanEngineOptions;

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;
  bool framebufferResized = false;
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;
};


#endif //VULKANPROJECT_VULKANENGINE_H
