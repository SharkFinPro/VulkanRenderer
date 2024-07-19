#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include "components/Window.h"
#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <glm/glm.hpp>

class DebugMessenger;
class Camera;
class Texture;
class Model;
class RenderObject;
class GraphicsPipeline;
class RenderPass;
class ImguiPipeline;

struct VulkanEngineOptions {
  uint32_t WINDOW_WIDTH;
  uint32_t WINDOW_HEIGHT;
  const char* WINDOW_TITLE;

  const char* VERTEX_SHADER_FILE;
  const char* FRAGMENT_SHADER_FILE;

  glm::vec3 cameraPosition = { 0.0f, 0.0f, 0.0f };
  float cameraSpeed = 1.0f;
};

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  [[nodiscard]] bool isComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions* vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path);
  std::shared_ptr<Model> loadModel(const char* path);
  std::shared_ptr<RenderObject> loadRenderObject(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> specularMap,
                                                 std::shared_ptr<Model>);

private:
  void initVulkan();
  void createInstance();
  static bool checkValidationLayerSupport();
  static std::vector<const char*> getRequiredExtensions();
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  void createLogicalDevice();
  static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
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
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  void createDepthResources();
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  VkSampleCountFlagBits getMaxUsableSampleCount();
  void createColorResources();

private:
  VulkanEngineOptions* vulkanEngineOptions;

  std::shared_ptr<Window> window;

  std::shared_ptr<Camera> camera;

  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;

  std::unique_ptr<DebugMessenger> debugMessenger;

  std::shared_ptr<RenderPass> renderPass;

  std::unique_ptr<GraphicsPipeline> graphicsPipeline;

  std::unique_ptr<ImguiPipeline> imguiPipeline;

  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkSurfaceKHR surface;
  VkQueue presentQueue;
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
  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;
};


#endif //VULKANPROJECT_VULKANENGINE_H
