#ifndef VULKANPROJECT_INSTANCE_H
#define VULKANPROJECT_INSTANCE_H

#include <array>
#include <vulkan/vulkan.h>
#include <vector>

#include <GLFW/glfw3.h>

constexpr std::array<const char*, 1> validationLayers {
  "VK_LAYER_KHRONOS_validation"
};

class Instance {
public:
  Instance();
  ~Instance();

  [[nodiscard]] VkSurfaceKHR createSurface(GLFWwindow* window) const;

  void destroySurface(VkSurfaceKHR& surface) const;

  [[nodiscard]] VkDebugUtilsMessengerEXT createDebugUtilsMessenger(const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) const;

  void destroyDebugUtilsMessenger(VkDebugUtilsMessengerEXT& debugMessenger) const;

  [[nodiscard]] std::vector<VkPhysicalDevice> getPhysicalDevices() const;

private:
  VkInstance instance = VK_NULL_HANDLE;

  static bool checkValidationLayerSupport();

  static std::vector<const char*> getRequiredExtensions();
};


#endif //VULKANPROJECT_INSTANCE_H
