#ifndef VKE_PHYSICALDEVICE_H
#define VKE_PHYSICALDEVICE_H

#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace vke {

  class Instance;
  class Surface;

  #ifdef __APPLE__
  constexpr std::array<const char*, 3> deviceExtensions {
    vk::KHRSwapchainExtensionName,
    vk::KHRDynamicRenderingExtensionName,
    "VK_KHR_portability_subset"
  };
  #else
  constexpr std::array deviceExtensions {
    vk::KHRSwapchainExtensionName,
    vk::KHRDynamicRenderingExtensionName
  };
  #endif

  constexpr std::array rayTracingDeviceExtensions {
    vk::KHRRayTracingPipelineExtensionName,
    vk::KHRAccelerationStructureExtensionName,
    vk::KHRBufferDeviceAddressExtensionName,
    vk::KHRDeferredHostOperationsExtensionName
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
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
  };

  class PhysicalDevice {
  public:
    PhysicalDevice(const std::shared_ptr<Instance>& instance,
                   std::shared_ptr<Surface> surface);

    [[nodiscard]] QueueFamilyIndices getQueueFamilies() const;

    [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;

    [[nodiscard]] vk::SampleCountFlagBits getMsaaSamples() const;

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter,
                                          const vk::MemoryPropertyFlags& properties) const;

    void updateSwapChainSupportDetails();

    [[nodiscard]] vk::FormatProperties getFormatProperties(vk::Format format) const;

    [[nodiscard]] vk::PhysicalDeviceProperties getDeviceProperties() const;

    [[nodiscard]] vk::raii::Device createLogicalDevice(const vk::DeviceCreateInfo& deviceCreateInfo) const;

    [[nodiscard]] vk::Format findDepthFormat() const;

    [[nodiscard]] vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                                 vk::ImageTiling tiling,
                                                 vk::FormatFeatureFlags features) const;

    [[nodiscard]] bool supportsRayTracing() const;

    [[nodiscard]] vk::PhysicalDeviceRayTracingPipelinePropertiesKHR getRayTracingPipelineProperties() const;

    friend class ImGuiInstance;

  private:
    vk::raii::PhysicalDevice m_physicalDevice = nullptr;

    std::shared_ptr<Surface> m_surface;

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    QueueFamilyIndices m_queueFamilyIndices;

    SwapChainSupportDetails m_swapChainSupportDetails;

    bool m_supportsRayTracing = false;

    void pickPhysicalDevice(const std::shared_ptr<Instance>& instance);

    [[nodiscard]] bool isDeviceSuitable(const vk::raii::PhysicalDevice& device) const;

    [[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device) const;

    static bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& device);

    static bool checkDeviceRayTracingExtensionSupport(const vk::raii::PhysicalDevice& device);

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& device) const;

    [[nodiscard]] vk::SampleCountFlagBits getMaxUsableSampleCount() const;
  };

} // namespace vke

#endif //VKE_PHYSICALDEVICE_H
