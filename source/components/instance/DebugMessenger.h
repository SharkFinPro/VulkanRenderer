#ifndef VKE_DEBUGMESSENGER_H
#define VKE_DEBUGMESSENGER_H

#include <vulkan/vulkan_raii.hpp>

namespace vke {

  class DebugMessenger {
  public:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData);

    static void populateCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

    static const char* readMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity);
  };

} // namespace vke

#endif //VKE_DEBUGMESSENGER_H
