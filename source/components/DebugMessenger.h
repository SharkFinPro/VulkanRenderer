#ifndef VULKANPROJECT_DEBUGMESSENGER_H
#define VULKANPROJECT_DEBUGMESSENGER_H

#include <vulkan/vulkan.h>
#include <memory>

class Instance;

class DebugMessenger {
public:
  explicit DebugMessenger(const std::shared_ptr<Instance>& instance);
  ~DebugMessenger();

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

  static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  static const char* readMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity);

private:
  VkDebugUtilsMessengerEXT debugMessenger;
  std::shared_ptr<Instance> instance;
};


#endif //VULKANPROJECT_DEBUGMESSENGER_H
