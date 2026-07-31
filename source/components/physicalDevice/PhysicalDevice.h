#ifndef VKE_PHYSICALDEVICE_H
#define VKE_PHYSICALDEVICE_H

#include "DeviceRequirements.h"
#include "../../EngineConfig.h"
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vke {

  class Instance;
  class Surface;

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

  // Everything one candidate device yields in a single gather pass, so nothing has to be
  // re-queried and the winner's data can be moved straight into the members.
  struct DeviceCandidate {
    vk::raii::PhysicalDevice device = nullptr;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    QueueFamilyIndices queueFamilyIndices;
    SwapChainSupportDetails swapChainSupport;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    vk::DeviceSize deviceLocalMemorySize = 0;
    bool supportsRayTracing = false;
    std::string rejectionReason;
    uint32_t score = 0;

    [[nodiscard]] bool isSuitable() const
    {
      return rejectionReason.empty();
    }
  };

  class PhysicalDevice {
  public:
    PhysicalDevice(const std::shared_ptr<Instance>& instance,
                   std::shared_ptr<Surface> surface,
                   const EngineConfig::Device& config);

    [[nodiscard]] QueueFamilyIndices getQueueFamilies() const;

    [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;

    [[nodiscard]] vk::SampleCountFlagBits getMsaaSamples() const;

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter,
                                          const vk::MemoryPropertyFlags& properties) const;

    void updateSwapChainSupportDetails();

    [[nodiscard]] vk::FormatProperties getFormatProperties(vk::Format format) const;

    [[nodiscard]] vk::PhysicalDeviceProperties getDeviceProperties() const;

    [[nodiscard]] std::string_view getDeviceName() const;

    [[nodiscard]] vk::PhysicalDeviceType getDeviceType() const;

    [[nodiscard]] vk::DeviceSize getDeviceLocalMemorySize() const;

    [[nodiscard]] std::string_view getDriverName() const;

    [[nodiscard]] std::string_view getDriverInfo() const;

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

    vk::PhysicalDeviceProperties m_properties;

    vk::PhysicalDeviceMemoryProperties m_memoryProperties;

    std::string m_deviceName;

    std::string m_driverName;

    std::string m_driverInfo;

    vk::DeviceSize m_deviceLocalMemorySize = 0;

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    QueueFamilyIndices m_queueFamilyIndices;

    SwapChainSupportDetails m_swapChainSupportDetails;

    bool m_supportsRayTracing = false;

    void pickPhysicalDevice(const std::shared_ptr<Instance>& instance,
                            const EngineConfig::Device& config);

    // Single gather pass. Never throws; an unsuitable device comes back with rejectionReason
    // set so the caller can report why it was skipped.
    [[nodiscard]] DeviceCandidate evaluateDevice(const vk::raii::PhysicalDevice& device) const;

    [[nodiscard]] static uint32_t scoreDevice(const DeviceCandidate& candidate);

    [[nodiscard]] static uint32_t deviceTypeScore(vk::PhysicalDeviceType deviceType);

    [[nodiscard]] static std::optional<std::size_t> selectBestCandidate(
      const std::vector<DeviceCandidate>& candidates,
      std::string_view preferredName);

    [[nodiscard]] static bool matchesPreferredName(std::string_view deviceName,
                                                  std::string_view preferredName);

    [[nodiscard]] static vk::DeviceSize findLargestDeviceLocalHeap(
      const vk::PhysicalDeviceMemoryProperties& memoryProperties);

    [[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device) const;

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& device) const;

    [[nodiscard]] static vk::SampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDeviceLimits& limits);

    void reportSelectedDevice() const;

    static void reportRejectedDevices(const std::vector<DeviceCandidate>& candidates);

    static void reportNoSuitableDevices(const std::vector<DeviceCandidate>& candidates);
  };

} // namespace vke

#endif //VKE_PHYSICALDEVICE_H
