#ifndef VULKANPROJECT_VULKANAPP_H
#define VULKANPROJECT_VULKANAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class VulkanApp {
public:
  void run();

private:
  void initVulkan();
  void createInstance();
  void mainLoop();
  void cleanup();
  void initWindow();
  static bool checkValidationLayerSupport();
  std::vector<const char*> getRequiredExtensions();
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
  void setupDebugMessenger();
  static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
  static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
  static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
  GLFWwindow* window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
};


#endif //VULKANPROJECT_VULKANAPP_H
