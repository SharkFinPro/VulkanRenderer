#include "DebugMessenger.h"
#include <iostream>

namespace vke {

  VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                               [[maybe_unused]] void* pUserData)
  {
    std::cerr << "[" << readMessageSeverity(messageSeverity) << "] validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

  void DebugMessenger::populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
  {
    createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback
    };
  }

  const char* DebugMessenger::readMessageSeverity(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
  {
    switch (messageSeverity)
    {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return "verbose";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return "info";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return "warning";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return "error";
      default:
        return "unknown";
    }
  }

} // namespace vke