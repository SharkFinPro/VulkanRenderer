#ifndef VULKANPROJECT_PHYSICALDEVICE_H
#define VULKANPROJECT_PHYSICALDEVICE_H

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  std::optional<uint32_t> computeFamily;

  [[nodiscard]] bool isComplete() const
  {
    return graphicsFamily.has_value() &&
           presentFamily.has_value() &&
           computeFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class PhysicalDevice {
public:
  PhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);

  VkPhysicalDevice& getPhysicalDevice();
  QueueFamilyIndices& getQueueFamilies();
  SwapChainSupportDetails& getSwapChainSupport();
  [[nodiscard]] VkSampleCountFlagBits getMsaaSamples() const;

private:
  VkPhysicalDevice physicalDevice;

  VkInstance& instance;
  VkSurfaceKHR& surface;

  VkSampleCountFlagBits msaaSamples;

  QueueFamilyIndices queueFamilyIndices;
  SwapChainSupportDetails swapChainSupportDetails;

  void pickPhysicalDevice();

  bool isDeviceSuitable(VkPhysicalDevice device) const;

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

  static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

  [[nodiscard]] VkSampleCountFlagBits getMaxUsableSampleCount() const;
};


#endif //VULKANPROJECT_PHYSICALDEVICE_H
