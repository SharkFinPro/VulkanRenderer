#include "PhysicalDevice.h"
#include "Instance.h"
#include <array>
#include <stdexcept>
#include <set>

PhysicalDevice::PhysicalDevice(const std::shared_ptr<Instance>& instance, VkSurfaceKHR& surface)
  : surface(surface), msaaSamples(VK_SAMPLE_COUNT_1_BIT)
{
  pickPhysicalDevice(instance);

  queueFamilyIndices = findQueueFamilies(physicalDevice);

  swapChainSupportDetails = querySwapChainSupport(physicalDevice);
}

VkPhysicalDevice PhysicalDevice::getPhysicalDevice() const
{
  return physicalDevice;
}

QueueFamilyIndices& PhysicalDevice::getQueueFamilies()
{
  return queueFamilyIndices;
}

SwapChainSupportDetails& PhysicalDevice::getSwapChainSupport()
{
  swapChainSupportDetails = querySwapChainSupport(physicalDevice);

  return swapChainSupportDetails;
}

VkSampleCountFlagBits PhysicalDevice::getMsaaSamples() const
{
  return msaaSamples;
}

uint32_t PhysicalDevice::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void PhysicalDevice::pickPhysicalDevice(const std::shared_ptr<Instance>& instance)
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance->getInstance(), &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance->getInstance(), &deviceCount, devices.data());

  for (const auto& device : devices)
  {
    if (isDeviceSuitable(device))
    {
      physicalDevice = device;
      msaaSamples = getMaxUsableSampleCount();
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
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
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

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

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(const VkPhysicalDevice device) const
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSampleCountFlagBits PhysicalDevice::getMaxUsableSampleCount() const
{
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

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
