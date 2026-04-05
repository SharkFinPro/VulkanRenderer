#include "DebugMessenger.h"
#include <iostream>

namespace vke {

  VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessenger::debugCallback(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                 [[maybe_unused]] void* pUserData)
  {
    std::cerr << "[" << readMessageSeverity(messageSeverity) << "] validation layer: " << pCallbackData->pMessage << std::endl;

    return vk::False;
  }

  void DebugMessenger::populateCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
  {
    createInfo = {
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = debugCallback
    };
  }

  const char* DebugMessenger::readMessageSeverity(const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
  {
    switch (messageSeverity)
    {
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        return "verbose";
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        return "info";
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        return "warning";
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        return "error";
      default:
        return "unknown";
    }
  }

} // namespace vke