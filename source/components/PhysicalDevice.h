#ifndef VULKANPROJECT_PHYSICALDEVICE_H
#define VULKANPROJECT_PHYSICALDEVICE_H

#include <vulkan/vulkan.h>
#include <optional>
#include <array>
#include <vector>
#include <memory>

class Instance;

constexpr std::array deviceExtensions {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
#ifdef __APPLE__
, "VK_KHR_portability_subset"
#endif
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
  PhysicalDevice(const std::shared_ptr<Instance>& instance, VkSurfaceKHR& surface);

  [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const;

  [[nodiscard]] QueueFamilyIndices getQueueFamilies() const;

  [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;

  [[nodiscard]] VkSampleCountFlagBits getMsaaSamples() const;

  [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const;

  void updateSwapChainSupportDetails();

private:
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  VkSurfaceKHR& surface;

  VkSampleCountFlagBits msaaSamples;

  QueueFamilyIndices queueFamilyIndices;

  SwapChainSupportDetails swapChainSupportDetails;

  void pickPhysicalDevice(const std::shared_ptr<Instance>& instance);

  bool isDeviceSuitable(VkPhysicalDevice device) const;

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

  static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

  [[nodiscard]] VkSampleCountFlagBits getMaxUsableSampleCount() const;
};


#endif //VULKANPROJECT_PHYSICALDEVICE_H
