#define GLFW_INCLUDE_VULKAN
#include "Instance.h"
#include "DebugMessenger.h"
#include <GLFW/glfw3.h>
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

    constexpr vk::ApplicationInfo appInfo {
      .pApplicationName = "Vulkan Engine",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = vk::ApiVersion13
    };

    const auto extensions = getRequiredExtensions();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    DebugMessenger::populateCreateInfo(debugCreateInfo);

    const vk::InstanceCreateInfo createInfo {
      .pNext = validationLayersEnabled() ? &debugCreateInfo : nullptr,
      .flags = IS_MAC ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR : vk::InstanceCreateFlags{},
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = validationLayersEnabled() ? static_cast<uint32_t>(validationLayers.size()) : 0,
      .ppEnabledLayerNames = validationLayersEnabled() ? validationLayers.data() : nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()
    };

    m_instance = vk::raii::Instance(m_context, createInfo);

    if (validationLayersEnabled())
    {
      createDebugUtilsMessenger();
    }
  }

  VkSurfaceKHR Instance::createSurface(GLFWwindow* window) const
  {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }

    return surface;
  }

  void Instance::destroySurface(VkSurfaceKHR& surface) const
  {
    vkDestroySurfaceKHR(*m_instance, surface, nullptr);

    surface = VK_NULL_HANDLE;
  }

  std::vector<vk::raii::PhysicalDevice> Instance::getPhysicalDevices() const
  {
    const auto devices = m_instance.enumeratePhysicalDevices();

    if (devices.empty())
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

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

  void Instance::createDebugUtilsMessenger()
  {
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    DebugMessenger::populateCreateInfo(debugCreateInfo);

    m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugCreateInfo);
  }

  bool Instance::checkValidationLayerSupport() const
  {
    const auto availableLayers = m_context.enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers)
    {
      bool layerFound = false;

      for (const auto& layerProperties : availableLayers)
      {
        if (std::string_view(layerName) == layerProperties.layerName)
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