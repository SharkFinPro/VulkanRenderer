#ifndef VULKANPROJECT_VULKANAPP_H
#define VULKANPROJECT_VULKANAPP_H

#include "Window.h"
#include "DebugMessenger.h"
#include "objects/RenderObject.h"
#include <vector>
#include <optional>
#include <string>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const std::string WINDOW_TITLE = "Vulkan";

const std::string MODEL_PATH = "assets/models/viking_room.obj";
const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

const std::string VERTEX_SHADER_FILE = "assets/shaders/vert.spv";
const std::string FRAGMENT_SHADER_FILE = "assets/shaders/frag.spv";

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

class VulkanApp {
public:
  void run();

private:
  void initVulkan();
  void createInstance();
  void mainLoop();
  void cleanup();
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
  void createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char>& code);
  void createRenderPass();
  void createFrameBuffers();
  void createCommandPool();
  void createCommandBuffers();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void drawFrame();
  void createSyncObjects();
  void cleanupSwapChain();
  void recreateSwapChain();
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  void createDescriptorSetLayout();
  void createTextureImage();
  void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                   VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                   VkImage& image, VkDeviceMemory& imageMemory);
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  void createTextureImageView();
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
  void createTextureSampler();
  void createDepthResources();
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  static bool hasStencilComponent(VkFormat format);
  void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
  VkSampleCountFlagBits getMaxUsableSampleCount();
  void createColorResources();

private:
  Window* window;
  std::vector<Model*> models;
  std::vector<RenderObject*> objects;

  VkInstance instance;
  DebugMessenger* debugMessenger;
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
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;
  bool framebufferResized = false;
  uint32_t mipLevels;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;
};


#endif //VULKANPROJECT_VULKANAPP_H
