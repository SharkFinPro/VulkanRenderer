#ifndef VULKANPROJECT_INSTANCE_H
#define VULKANPROJECT_INSTANCE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <array>
#include <vector>

constexpr std::array<const char*, 1> validationLayers {
  "VK_LAYER_KHRONOS_validation"
};

class Instance {
public:
  Instance();
  ~Instance();

  [[nodiscard]] VkSurfaceKHR createSurface(GLFWwindow* window) const;

  void destroySurface(VkSurfaceKHR& surface) const;

  void createDebugUtilsMessenger();

  void destroyDebugUtilsMessenger();

  [[nodiscard]] std::vector<VkPhysicalDevice> getPhysicalDevices() const;

  friend class ImGuiInstance;

private:
  VkInstance m_instance = VK_NULL_HANDLE;

  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  static bool checkValidationLayerSupport();

  static std::vector<const char*> getRequiredExtensions();
};


#endif //VULKANPROJECT_INSTANCE_H
