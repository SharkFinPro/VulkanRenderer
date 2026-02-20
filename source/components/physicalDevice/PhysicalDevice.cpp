#include "PhysicalDevice.h"
#include "../instance/Instance.h"
#include "../window/Surface.h"
#include <array>
#include <set>
#include <stdexcept>

namespace vke {
  PhysicalDevice::PhysicalDevice(const std::shared_ptr<Instance>& instance,
                                 std::shared_ptr<Surface> surface)
    : m_surface(std::move(surface))
  {
    pickPhysicalDevice(instance);

    m_queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    updateSwapChainSupportDetails();
  }

  QueueFamilyIndices PhysicalDevice::getQueueFamilies() const
  {
    return m_queueFamilyIndices;
  }

  SwapChainSupportDetails PhysicalDevice::getSwapChainSupport() const
  {
    return m_swapChainSupportDetails;
  }

  VkSampleCountFlagBits PhysicalDevice::getMsaaSamples() const
  {
    return m_msaaSamples;
  }

  uint32_t PhysicalDevice::findMemoryType(const uint32_t typeFilter,
                                          const VkMemoryPropertyFlags& properties) const
  {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
      if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  void PhysicalDevice::updateSwapChainSupportDetails()
  {
    m_swapChainSupportDetails = querySwapChainSupport(m_physicalDevice);
  }

  VkFormatProperties PhysicalDevice::getFormatProperties(const VkFormat format) const
  {
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);

    return formatProperties;
  }

  VkPhysicalDeviceProperties PhysicalDevice::getDeviceProperties() const
  {
    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);

    return deviceProperties;
  }

  VkDevice PhysicalDevice::createLogicalDevice(const VkDeviceCreateInfo& deviceCreateInfo) const
  {
    VkDevice logicalDevice;

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create logical device!");
    }

    return logicalDevice;
  }

  VkFormat PhysicalDevice::findDepthFormat() const
  {
    return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }

  VkFormat PhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                               const VkImageTiling tiling,
                                               const VkFormatFeatureFlags features) const
  {
    for (const auto& format : candidates)
    {
      const VkFormatProperties formatProperties = getFormatProperties(format);

      if ((tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) ||
          (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features))
      {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  bool PhysicalDevice::supportsRayTracing() const
  {
    return m_supportsRayTracing;
  }

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR PhysicalDevice::getRayTracingPipelineProperties() const
  {
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
    };

    VkPhysicalDeviceProperties2 physicalDeviceProperties {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &rayTracingPipelineProperties
    };

    vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties);

    return rayTracingPipelineProperties;
  }

  void PhysicalDevice::pickPhysicalDevice(const std::shared_ptr<Instance>& instance)
  {
    for (const auto& device : instance->getPhysicalDevices())
    {
      if (isDeviceSuitable(device))
      {
        m_physicalDevice = device;
        m_msaaSamples = getMaxUsableSampleCount();
        break;
      }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }

    m_supportsRayTracing = checkDeviceRayTracingExtensionSupport(m_physicalDevice);
  }

  bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) const
  {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
  }

  QueueFamilyIndices PhysicalDevice::findQueueFamilies(const VkPhysicalDevice device) const
  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphicsFamily = i;
      }

      if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
      {
        indices.computeFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface->getSurface(), &presentSupport);

      if (presentSupport)
      {
        indices.presentFamily = i;
      }

      if (indices.isComplete())
      {
        break;
      }

      i++;
    }

    return indices;
  }

  bool PhysicalDevice::checkDeviceExtensionSupport(const VkPhysicalDevice device)
  {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  bool PhysicalDevice::checkDeviceRayTracingExtensionSupport(VkPhysicalDevice device)
  {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(rayTracingDeviceExtensions.begin(), rayTracingDeviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(const VkPhysicalDevice device) const
  {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface->getSurface(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface->getSurface(), &formatCount, nullptr);

    if (formatCount != 0)
    {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface->getSurface(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface->getSurface(), &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface->getSurface(), &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  VkSampleCountFlagBits PhysicalDevice::getMaxUsableSampleCount() const
  {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                      physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    constexpr std::array<VkSampleCountFlagBits, 6> sampleCounts {
      VK_SAMPLE_COUNT_64_BIT,
      VK_SAMPLE_COUNT_32_BIT,
      VK_SAMPLE_COUNT_16_BIT,
      VK_SAMPLE_COUNT_8_BIT,
      VK_SAMPLE_COUNT_4_BIT,
      VK_SAMPLE_COUNT_2_BIT
    };

    for (const VkSampleCountFlagBits count : sampleCounts)
    {
      if (counts & count)
      {
        return count;
      }
    }

    return VK_SAMPLE_COUNT_1_BIT;
  }

} // namespace vke