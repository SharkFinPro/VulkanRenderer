#define GLFW_INCLUDE_VULKAN
#include "Instance.h"
#include "DebugMessenger.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#ifdef __APPLE__
inline constexpr bool IS_MAC = true;
#else
inline constexpr bool IS_MAC = false;
#endif

namespace vke {

  Instance::Instance()
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (validationLayersEnabled() && !checkValidationLayerSupport())
    {
      throw std::runtime_error("validation layers requested, but not available!");
    }

    constexpr vk::ApplicationInfo appInfo {
      .pApplicationName = "Vulkan Engine",
      .applicationVersion = vk::makeApiVersion(1, 0, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = vk::makeApiVersion(1, 0, 0, 0),
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

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

    if (validationLayersEnabled())
    {
      createDebugUtilsMessenger();
    }
  }

  vk::raii::SurfaceKHR Instance::createSurface(GLFWwindow* window) const
  {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }

    return vk::raii::SurfaceKHR(m_instance, surface);
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

    return std::ranges::all_of(validationLayers, [&](const auto& layerName) {
      return std::ranges::any_of(availableLayers, [&](const auto& layerProperties) {
        return std::string_view(layerName) == layerProperties.layerName;
      });
    });
  }

  std::vector<const char*> Instance::getRequiredExtensions()
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (validationLayersEnabled())
    {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    if (IS_MAC)
    {
      extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    }

    return extensions;
  }

} // namespace vke