#ifndef VKE_PHYSICALDEVICE_H
#define VKE_PHYSICALDEVICE_H

#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace vke {

  class Instance;

  #ifdef __APPLE__
  constexpr std::array<const char*, 3> deviceExtensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    "VK_KHR_portability_subset"
  };
  #else
  constexpr std::array<const char*, 2> deviceExtensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
  };
  #endif

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
    VkSurfaceCapabilitiesKHR capabilities {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  class PhysicalDevice {
  public:
    PhysicalDevice(const std::shared_ptr<Instance>& instance, VkSurfaceKHR& surface);

    [[nodiscard]] QueueFamilyIndices getQueueFamilies() const;

    [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;

    [[nodiscard]] VkSampleCountFlagBits getMsaaSamples() const;

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter,
                                          const VkMemoryPropertyFlags& properties) const;

    void updateSwapChainSupportDetails();

    [[nodiscard]] VkFormatProperties getFormatProperties(VkFormat format) const;

    [[nodiscard]] VkPhysicalDeviceProperties getDeviceProperties() const;

    [[nodiscard]] VkDevice createLogicalDevice(const VkDeviceCreateInfo& deviceCreateInfo) const;

    [[nodiscard]] VkFormat findDepthFormat() const;

    [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                               VkImageTiling tiling,
                                               VkFormatFeatureFlags features) const;

    friend class ImGuiInstance;

  private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    VkSurfaceKHR& m_surface;

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    QueueFamilyIndices m_queueFamilyIndices;

    SwapChainSupportDetails m_swapChainSupportDetails;

    void pickPhysicalDevice(const std::shared_ptr<Instance>& instance);

    bool isDeviceSuitable(VkPhysicalDevice device) const;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

    [[nodiscard]] VkSampleCountFlagBits getMaxUsableSampleCount() const;
  };

} // namespace vke

#endif //VKE_PHYSICALDEVICE_H
