#ifndef VULKANPROJECT_DEBUGMESSENGER_H
#define VULKANPROJECT_DEBUGMESSENGER_H

#include <vulkan/vulkan.h>

class DebugMessenger {
public:
  explicit DebugMessenger(VkInstance* instance);
  ~DebugMessenger();

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

  static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
  VkDebugUtilsMessengerEXT debugMessenger;
  VkInstance* instance;
};


#endif //VULKANPROJECT_DEBUGMESSENGER_H
