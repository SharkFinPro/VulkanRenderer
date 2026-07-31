#ifndef VKE_DEVICEREQUIREMENTS_H
#define VKE_DEVICEREQUIREMENTS_H

#include <vulkan/vulkan_raii.hpp>
#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace vke {

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

  // One required Vulkan feature bit, located by pointer-to-member inside the feature struct
  // that owns it. PhysicalDevice reads these bits off a candidate device to decide whether it
  // is suitable; LogicalDevice::createDevice() writes the same bits into the chain it requests.
  // Neither side names a bit directly, so the two cannot drift apart.
  template <typename FeaturesT>
  struct DeviceFeatureRequirement {
    std::string_view name;
    vk::Bool32 FeaturesT::* member;
  };

  constexpr std::array requiredCoreFeatures {
    DeviceFeatureRequirement<vk::PhysicalDeviceFeatures>{
      .name = "geometryShader", .member = &vk::PhysicalDeviceFeatures::geometryShader },
    DeviceFeatureRequirement<vk::PhysicalDeviceFeatures>{
      .name = "fillModeNonSolid", .member = &vk::PhysicalDeviceFeatures::fillModeNonSolid },
    DeviceFeatureRequirement<vk::PhysicalDeviceFeatures>{
      .name = "samplerAnisotropy", .member = &vk::PhysicalDeviceFeatures::samplerAnisotropy }
  };

  constexpr std::array requiredVulkan11Features {
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan11Features>{
      .name = "multiview", .member = &vk::PhysicalDeviceVulkan11Features::multiview }
  };

  constexpr std::array requiredVulkan12Features {
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "shaderSampledImageArrayNonUniformIndexing",
      .member = &vk::PhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "descriptorBindingPartiallyBound",
      .member = &vk::PhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "runtimeDescriptorArray",
      .member = &vk::PhysicalDeviceVulkan12Features::runtimeDescriptorArray },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "timelineSemaphore",
      .member = &vk::PhysicalDeviceVulkan12Features::timelineSemaphore }
  };

  constexpr std::array requiredVulkan13Features {
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan13Features>{
      .name = "shaderDemoteToHelperInvocation",
      .member = &vk::PhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan13Features>{
      .name = "synchronization2",
      .member = &vk::PhysicalDeviceVulkan13Features::synchronization2 },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan13Features>{
      .name = "dynamicRendering",
      .member = &vk::PhysicalDeviceVulkan13Features::dynamicRendering }
  };

  // Requested only when the ray tracing extensions are present, so a device missing these is
  // still suitable — it just runs without ray tracing.
  constexpr std::array rayTracingVulkan12Features {
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "descriptorBindingVariableDescriptorCount",
      .member = &vk::PhysicalDeviceVulkan12Features::descriptorBindingVariableDescriptorCount },
    DeviceFeatureRequirement<vk::PhysicalDeviceVulkan12Features>{
      .name = "bufferDeviceAddress",
      .member = &vk::PhysicalDeviceVulkan12Features::bufferDeviceAddress }
  };

  constexpr std::array rayTracingAccelerationStructureFeatures {
    DeviceFeatureRequirement<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>{
      .name = "accelerationStructure",
      .member = &vk::PhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure }
  };

  constexpr std::array rayTracingPipelineFeatures {
    DeviceFeatureRequirement<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>{
      .name = "rayTracingPipeline",
      .member = &vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline }
  };

  // Name of the first unsupported requirement, or an empty view when all are supported.
  template <typename FeaturesT, std::size_t N>
  [[nodiscard]] constexpr std::string_view findMissingFeature(
    const FeaturesT& supportedFeatures,
    const std::array<DeviceFeatureRequirement<FeaturesT>, N>& requirements)
  {
    const auto missing = std::ranges::find_if(requirements, [&supportedFeatures](const auto& requirement) {
      return supportedFeatures.*requirement.member == vk::False;
    });

    return missing == requirements.end() ? std::string_view{} : missing->name;
  }

  template <typename FeaturesT, std::size_t N>
  constexpr void requestFeatures(FeaturesT& requestedFeatures,
                                 const std::array<DeviceFeatureRequirement<FeaturesT>, N>& requirements)
  {
    for (const auto& requirement : requirements)
    {
      requestedFeatures.*requirement.member = vk::True;
    }
  }

  // Name of the first missing extension, or an empty view when all are present. Takes the
  // already enumerated extensions so a caller checking several lists only enumerates once.
  [[nodiscard]] inline std::string_view findMissingExtension(
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const std::span<const char* const> requiredExtensions)
  {
    for (const char* const required : requiredExtensions)
    {
      const bool available = std::ranges::any_of(availableExtensions,
        [required](const vk::ExtensionProperties& extension)
        {
          return std::string_view(required) == extension.extensionName;
        });

      if (!available)
      {
        return required;
      }
    }

    return {};
  }

} // namespace vke

#endif //VKE_DEVICEREQUIREMENTS_H
