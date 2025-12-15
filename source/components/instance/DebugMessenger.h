#ifndef VKE_DEBUGMESSENGER_H
#define VKE_DEBUGMESSENGER_H

#include <vulkan/vulkan.h>

namespace vke {

class DebugMessenger {
public:
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

  static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  static const char* readMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity);
};

} // namespace vke

#endif //VKE_DEBUGMESSENGER_H
