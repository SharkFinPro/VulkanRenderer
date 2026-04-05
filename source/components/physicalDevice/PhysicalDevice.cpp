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

  vk::SampleCountFlagBits PhysicalDevice::getMsaaSamples() const
  {
    return m_msaaSamples;
  }

  uint32_t PhysicalDevice::findMemoryType(const uint32_t typeFilter,
                                          const vk::MemoryPropertyFlags& properties) const
  {
    const auto memoryProperties = m_physicalDevice.getMemoryProperties();

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

  vk::FormatProperties PhysicalDevice::getFormatProperties(const vk::Format format) const
  {
    return m_physicalDevice.getFormatProperties(format);
  }

  vk::PhysicalDeviceProperties PhysicalDevice::getDeviceProperties() const
  {
    return m_physicalDevice.getProperties();
  }

  vk::raii::Device PhysicalDevice::createLogicalDevice(const vk::DeviceCreateInfo& deviceCreateInfo) const
  {
    return m_physicalDevice.createDevice(deviceCreateInfo);
  }

  vk::Format PhysicalDevice::findDepthFormat() const
  {
    return findSupportedFormat(
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
  }

  vk::Format PhysicalDevice::findSupportedFormat(const std::vector<vk::Format>& candidates,
                                               const vk::ImageTiling tiling,
                                               const vk::FormatFeatureFlags features) const
  {
    for (const auto& format : candidates)
    {
      const vk::FormatProperties formatProperties = getFormatProperties(format);

      if ((tiling == vk::ImageTiling::eLinear && (formatProperties.linearTilingFeatures & features) == features) ||
          (tiling == vk::ImageTiling::eOptimal && (formatProperties.optimalTilingFeatures & features) == features))
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

  vk::PhysicalDeviceRayTracingPipelinePropertiesKHR PhysicalDevice::getRayTracingPipelineProperties() const
  {
    return m_physicalDevice.getProperties2<
      vk::PhysicalDeviceProperties2,
      vk::PhysicalDeviceRayTracingPipelinePropertiesKHR
    >().get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
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

    if (!*m_physicalDevice)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }

    m_supportsRayTracing = checkDeviceRayTracingExtensionSupport(m_physicalDevice);
  }

  bool PhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& device) const
  {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    const auto supportedFeatures = device.getFeatures();

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
  }

  QueueFamilyIndices PhysicalDevice::findQueueFamilies(const vk::raii::PhysicalDevice& device) const
  {
    QueueFamilyIndices indices;

    const auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
      if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
      {
        indices.graphicsFamily = i;
      }

      if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
      {
        indices.computeFamily = i;
      }

      if (device.getSurfaceSupportKHR(i, m_surface->getSurface()))
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

  bool PhysicalDevice::checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& device)
  {
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  bool PhysicalDevice::checkDeviceRayTracingExtensionSupport(const vk::raii::PhysicalDevice& device)
  {
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(rayTracingDeviceExtensions.begin(), rayTracingDeviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(const vk::raii::PhysicalDevice& device) const
  {
    const auto surface = m_surface->getSurface();

    return {
      .capabilities = device.getSurfaceCapabilitiesKHR(surface),
      .formats = device.getSurfaceFormatsKHR(surface),
      .presentModes = device.getSurfacePresentModesKHR(surface)
    };
  }

  vk::SampleCountFlagBits PhysicalDevice::getMaxUsableSampleCount() const
  {
    const auto physicalDeviceProperties = m_physicalDevice.getProperties();

    const vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                        physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    constexpr std::array sampleCounts {
      vk::SampleCountFlagBits::e64,
      vk::SampleCountFlagBits::e32,
      vk::SampleCountFlagBits::e16,
      vk::SampleCountFlagBits::e8,
      vk::SampleCountFlagBits::e4,
      vk::SampleCountFlagBits::e2
    };

    for (const vk::SampleCountFlagBits count : sampleCounts)
    {
      if (counts & count)
      {
        return count;
      }
    }

    return vk::SampleCountFlagBits::e1;
  }

} // namespace vke