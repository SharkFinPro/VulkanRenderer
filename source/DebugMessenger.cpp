#include "DebugMessenger.h"
#include <iostream>

DebugMessenger::DebugMessenger(VkInstance* instance)
  : instance(instance)
{
  VkResult result;
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  populateCreateInfo(debugCreateInfo);

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr)
  {
    result = func(*instance, &debugCreateInfo, nullptr, &debugMessenger);
  }
  else
  {
    result = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

DebugMessenger::~DebugMessenger()
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
  {
    func(*instance, debugMessenger, nullptr);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData)
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

void DebugMessenger::populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}
