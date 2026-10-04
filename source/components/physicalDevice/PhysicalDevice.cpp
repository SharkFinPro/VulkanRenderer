#include "PhysicalDevice.h"
#include "../instance/Instance.h"
#include "../window/Surface.h"
#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

namespace vke {

  namespace {
    template <typename Features>
    struct FeatureSpec {
      const char* name;
      vk::Bool32 Features::* member;
    };

    // The features the engine requires, tables shared by the suitability check and the device creation.
    constexpr std::array coreFeatures {
      FeatureSpec<vk::PhysicalDeviceFeatures>{ "geometryShader", &vk::PhysicalDeviceFeatures::geometryShader },
      FeatureSpec<vk::PhysicalDeviceFeatures>{ "fillModeNonSolid", &vk::PhysicalDeviceFeatures::fillModeNonSolid },
      FeatureSpec<vk::PhysicalDeviceFeatures>{ "samplerAnisotropy", &vk::PhysicalDeviceFeatures::samplerAnisotropy }
    };

    constexpr std::array vulkan11Features {
      FeatureSpec<vk::PhysicalDeviceVulkan11Features>{ "multiview", &vk::PhysicalDeviceVulkan11Features::multiview }
    };

    constexpr std::array vulkan12Features {
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "shaderSampledImageArrayNonUniformIndexing", &vk::PhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing },
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "descriptorBindingPartiallyBound", &vk::PhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound },
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "runtimeDescriptorArray", &vk::PhysicalDeviceVulkan12Features::runtimeDescriptorArray },
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "timelineSemaphore", &vk::PhysicalDeviceVulkan12Features::timelineSemaphore }
    };

    constexpr std::array vulkan13Features {
      FeatureSpec<vk::PhysicalDeviceVulkan13Features>{ "shaderDemoteToHelperInvocation", &vk::PhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation },
      FeatureSpec<vk::PhysicalDeviceVulkan13Features>{ "synchronization2", &vk::PhysicalDeviceVulkan13Features::synchronization2 },
      FeatureSpec<vk::PhysicalDeviceVulkan13Features>{ "dynamicRendering", &vk::PhysicalDeviceVulkan13Features::dynamicRendering }
    };

    // Needed only for ray tracing.
    constexpr std::array rayTracingVulkan12Features {
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "descriptorBindingVariableDescriptorCount", &vk::PhysicalDeviceVulkan12Features::descriptorBindingVariableDescriptorCount },
      FeatureSpec<vk::PhysicalDeviceVulkan12Features>{ "bufferDeviceAddress", &vk::PhysicalDeviceVulkan12Features::bufferDeviceAddress }
    };

    constexpr std::array accelerationStructureFeatures {
      FeatureSpec<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>{ "accelerationStructure", &vk::PhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure }
    };

    constexpr std::array rayTracingPipelineFeatures {
      FeatureSpec<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>{ "rayTracingPipeline", &vk::PhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline }
    };

    template <typename Features, size_t N>
    const char* findMissingFeature(const std::array<FeatureSpec<Features>, N>& specs, const Features& supported)
    {
      for (const auto& spec : specs)
      {
        if (!(supported.*spec.member))
        {
          return spec.name;
        }
      }

      return nullptr;
    }

    template <typename Features, size_t N>
    void enableFeatures(const std::array<FeatureSpec<Features>, N>& specs, Features& features)
    {
      for (const auto& spec : specs)
      {
        features.*spec.member = vk::True;
      }
    }

    int rankDeviceType(const vk::PhysicalDeviceType type)
    {
      switch (type)
      {
        case vk::PhysicalDeviceType::eDiscreteGpu:
          return 0;
        case vk::PhysicalDeviceType::eIntegratedGpu:
          return 1;
        default:
          return 2;
      }
    }
  } // namespace
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

  GpuCapabilities PhysicalDevice::getCapabilities() const
  {
    const auto properties = m_physicalDevice.getProperties();

    return {
      .deviceName = properties.deviceName,
      .deviceType = properties.deviceType,
      .apiVersion = properties.apiVersion,
      .rayTracing = m_supportsRayTracing,
      .msaaSamples = m_msaaSamples,
      .depthFormat = findDepthFormat()
    };
  }

  PhysicalDevice::FeatureChain PhysicalDevice::makeEnabledFeatures(const bool rayTracing)
  {
    FeatureChain chain;

    enableFeatures(coreFeatures, chain.get<vk::PhysicalDeviceFeatures2>().features);
    enableFeatures(vulkan11Features, chain.get<vk::PhysicalDeviceVulkan11Features>());
    enableFeatures(vulkan12Features, chain.get<vk::PhysicalDeviceVulkan12Features>());
    enableFeatures(vulkan13Features, chain.get<vk::PhysicalDeviceVulkan13Features>());

    if (rayTracing)
    {
      enableFeatures(rayTracingVulkan12Features, chain.get<vk::PhysicalDeviceVulkan12Features>());
      enableFeatures(accelerationStructureFeatures, chain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>());
      enableFeatures(rayTracingPipelineFeatures, chain.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>());
    }
    else
    {
      chain.unlink<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
      chain.unlink<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
    }

    return chain;
  }

  void PhysicalDevice::pickPhysicalDevice(const std::shared_ptr<Instance>& instance)
  {
    struct Candidate {
      vk::raii::PhysicalDevice device;
      int typeRank;
      bool rayTracing;
    };

    std::optional<Candidate> best;
    std::string rejections;

    for (const auto& device : instance->getPhysicalDevices())
    {
      const auto properties = device.getProperties();

      if (const auto reason = findRejectionReason(device))
      {
        rejections += std::format("\n  {}: {}", properties.deviceName.data(), *reason);
        continue;
      }

      const Candidate candidate {
        .device = device,
        .typeRank = rankDeviceType(properties.deviceType),
        .rayTracing = checkRayTracingSupport(device)
      };

      // Discrete before integrated before the rest, then ray tracing; otherwise the first one found stays.
      if (!best ||
          candidate.typeRank < best->typeRank ||
          (candidate.typeRank == best->typeRank && candidate.rayTracing && !best->rayTracing))
      {
        best = candidate;
      }
    }

    if (!best)
    {
      throw std::runtime_error("failed to find a suitable GPU:" + rejections);
    }

    m_physicalDevice = best->device;
    m_supportsRayTracing = best->rayTracing;
    m_msaaSamples = getMaxUsableSampleCount();
  }

  std::optional<std::string> PhysicalDevice::findRejectionReason(const vk::raii::PhysicalDevice& device) const
  {
    const auto properties = device.getProperties();

    if (properties.apiVersion < vk::ApiVersion13)
    {
      return std::format("supports Vulkan {}.{}, 1.3 is required",
                         vk::apiVersionMajor(properties.apiVersion),
                         vk::apiVersionMinor(properties.apiVersion));
    }

    if (!findQueueFamilies(device).isComplete())
    {
      return "no graphics, compute and present queue families";
    }

    if (const auto missing = findMissingExtension(device, deviceExtensions); !missing.empty())
    {
      return "missing extension " + missing;
    }

    const auto swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
    {
      return "the surface offers no swapchain formats or present modes";
    }

    const auto features = device.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan12Features,
      vk::PhysicalDeviceVulkan13Features
    >();

    const char* missing = findMissingFeature(coreFeatures, features.get<vk::PhysicalDeviceFeatures2>().features);
    missing = missing ? missing : findMissingFeature(vulkan11Features, features.get<vk::PhysicalDeviceVulkan11Features>());
    missing = missing ? missing : findMissingFeature(vulkan12Features, features.get<vk::PhysicalDeviceVulkan12Features>());
    missing = missing ? missing : findMissingFeature(vulkan13Features, features.get<vk::PhysicalDeviceVulkan13Features>());

    if (missing)
    {
      return std::string("missing feature ") + missing;
    }

    return std::nullopt;
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

  std::string PhysicalDevice::findMissingExtension(const vk::raii::PhysicalDevice& device,
                                                   const std::span<const char* const> extensions)
  {
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();

    for (const char* required : extensions)
    {
      const bool available = std::ranges::any_of(availableExtensions, [required](const auto& extension) {
        return std::string_view(extension.extensionName) == required;
      });

      if (!available)
      {
        return required;
      }
    }

    return {};
  }

  bool PhysicalDevice::checkRayTracingSupport(const vk::raii::PhysicalDevice& device)
  {
    if (!findMissingExtension(device, rayTracingDeviceExtensions).empty())
    {
      return false;
    }

    const auto features = device.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan12Features,
      vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
      vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
    >();

    return !findMissingFeature(rayTracingVulkan12Features, features.get<vk::PhysicalDeviceVulkan12Features>()) &&
           !findMissingFeature(accelerationStructureFeatures, features.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>()) &&
           !findMissingFeature(rayTracingPipelineFeatures, features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>());
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