#define GLFW_INCLUDE_VULKAN
#include "Instance.h"
#include "DebugMessenger.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <stdexcept>

#ifdef __APPLE__
#define IS_MAC 1
#else
#define IS_MAC 0
#endif

namespace vke {

  Instance::Instance()
  {
    if (validationLayersEnabled() && !checkValidationLayerSupport())
    {
      throw std::runtime_error("validation layers requested, but not available!");
    }

    constexpr VkApplicationInfo appInfo {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Vulkan Engine",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3
    };

    const auto extensions = getRequiredExtensions();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationLayersEnabled())
    {
      DebugMessenger::populateCreateInfo(debugCreateInfo);
    }

    const VkInstanceCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = validationLayersEnabled() ? &debugCreateInfo : nullptr,
      .flags = IS_MAC ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = validationLayersEnabled() ? static_cast<uint32_t>(validationLayers.size()) : 0,
      .ppEnabledLayerNames = validationLayersEnabled() ? validationLayers.data() : nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()
    };

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create instance!");
    }

    if (validationLayersEnabled())
    {
      createDebugUtilsMessenger();
    }
  }

  Instance::~Instance()
  {
    destroyDebugUtilsMessenger();

    vkDestroyInstance(m_instance, nullptr);

    m_instance = VK_NULL_HANDLE;
  }

  VkSurfaceKHR Instance::createSurface(GLFWwindow* window) const
  {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(m_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }

    return surface;
  }

  void Instance::destroySurface(VkSurfaceKHR& surface) const
  {
    vkDestroySurfaceKHR(m_instance, surface, nullptr);

    surface = VK_NULL_HANDLE;
  }

  void Instance::createDebugUtilsMessenger()
  {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    DebugMessenger::populateCreateInfo(debugCreateInfo);

    VkResult result;

    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
      result = func(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger);
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

  void Instance::destroyDebugUtilsMessenger()
  {
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
      func(m_instance, m_debugMessenger, nullptr);
    }

    m_debugMessenger = VK_NULL_HANDLE;
  }

  std::vector<VkPhysicalDevice> Instance::getPhysicalDevices() const
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    return devices;
  }

  bool Instance::validationLayersEnabled()
  {
  #ifdef NDEBUG
    return false;
  #else
    return true;
  #endif
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

    if (validationLayersEnabled())
    {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if constexpr (IS_MAC)
    {
      extensions.push_back("VK_KHR_portability_enumeration");
    }

    return extensions;
  }

} // namespace vke