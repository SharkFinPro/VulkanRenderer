#include "Instance.h"
#include "DebugMessenger.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cstring>


#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

#ifdef __APPLE__
#define IS_MAC 1
#else
#define IS_MAC 0
#endif

Instance::Instance()
{
  if (enableValidationLayers && !checkValidationLayerSupport())
  {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  constexpr VkApplicationInfo appInfo {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Vulkan Engine",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_1
  };

  const auto extensions = getRequiredExtensions();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers)
  {
    DebugMessenger::populateCreateInfo(debugCreateInfo);
  }

  const VkInstanceCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = enableValidationLayers ? &debugCreateInfo : nullptr,
    .flags = IS_MAC ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
    .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data()
  };

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create instance!");
  }
}

Instance::~Instance()
{
  vkDestroyInstance(instance, nullptr);
}

VkSurfaceKHR Instance::createSurface(GLFWwindow* window) const
{
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }

  return surface;
}

void Instance::destroySurface(VkSurfaceKHR& surface) const
{
  vkDestroySurfaceKHR(instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
}

VkDebugUtilsMessengerEXT Instance::createDebugUtilsMessenger(const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) const
{
  VkDebugUtilsMessengerEXT debugMessenger;
  VkResult result;

  const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr)
  {
    result = func(instance, &debugCreateInfo, nullptr, &debugMessenger);
  }
  else
  {
    result = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to set up debug messenger!");
  }

  return debugMessenger;
}

void Instance::destroyDebugUtilsMessenger(VkDebugUtilsMessengerEXT& debugMessenger) const
{
  const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr)
  {
    func(instance, debugMessenger, nullptr);
  }

  debugMessenger = VK_NULL_HANDLE;
}

bool Instance::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers)
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
    {
      return false;
    }
  }

  return true;
}

std::vector<const char*> Instance::getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if constexpr (IS_MAC)
  {
    extensions.push_back("VK_KHR_portability_enumeration");
  }

  return extensions;
}