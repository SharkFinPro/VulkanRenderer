#include "PhysicalDevice.h"
#include "../instance/Instance.h"
#include "../window/Surface.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace vke {

  namespace {
    constexpr uint32_t deviceTypeScoreDiscreteGpu   = 50'000;
    constexpr uint32_t deviceTypeScoreIntegratedGpu = 40'000;
    constexpr uint32_t deviceTypeScoreVirtualGpu    = 30'000;
    constexpr uint32_t deviceTypeScoreCpu           = 20'000;
    constexpr uint32_t deviceTypeScoreOther         = 10'000;
    constexpr uint32_t deviceTypeScoreStride        = 10'000;

    constexpr uint32_t rayTracingScore      = 2'000;
    constexpr uint32_t heapScoreMibPerPoint = 256;
    constexpr uint32_t heapScoreMax         = 1'023;  // saturates at 256 GiB of device-local memory
    constexpr uint32_t msaaScoreMax         = 64;
    constexpr uint32_t apiMinorScoreMax     = 15;

    constexpr vk::DeviceSize bytesPerMib = 1'024 * 1'024;

    // Every non-type term is clamped, so the tiebreaker budget is a fixed number that is
    // provably smaller than the gap between adjacent device-type tiers. Without this, a CPU
    // rasterizer exposing a large "device-local" system-RAM heap could outrank a virtual GPU.
    static_assert(rayTracingScore + heapScoreMax + msaaScoreMax + apiMinorScoreMax < deviceTypeScoreStride,
                  "tiebreakers must never be able to promote a device past its type tier");

    // Within one tier, ray tracing outranks every remaining term combined.
    static_assert(heapScoreMax + msaaScoreMax + apiMinorScoreMax < rayTracingScore,
                  "ray tracing support must dominate the memory/MSAA/API tiebreakers");
  }

  PhysicalDevice::PhysicalDevice(const std::shared_ptr<Instance>& instance,
                                 std::shared_ptr<Surface> surface,
                                 const EngineConfig::Device& config)
    : m_surface(std::move(surface))
  {
    pickPhysicalDevice(instance, config);

    reportSelectedDevice();
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
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
      if (typeFilter & (1 << i) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
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
    return m_properties;
  }

  std::string_view PhysicalDevice::getDeviceName() const
  {
    return m_deviceName;
  }

  vk::PhysicalDeviceType PhysicalDevice::getDeviceType() const
  {
    return m_properties.deviceType;
  }

  vk::DeviceSize PhysicalDevice::getDeviceLocalMemorySize() const
  {
    return m_deviceLocalMemorySize;
  }

  std::string_view PhysicalDevice::getDriverName() const
  {
    return m_driverName;
  }

  std::string_view PhysicalDevice::getDriverInfo() const
  {
    return m_driverInfo;
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

  void PhysicalDevice::pickPhysicalDevice(const std::shared_ptr<Instance>& instance,
                                          const EngineConfig::Device& config)
  {
    const auto devices = instance->getPhysicalDevices();

    std::vector<DeviceCandidate> candidates;
    candidates.reserve(devices.size());

    for (const auto& device : devices)
    {
      candidates.push_back(evaluateDevice(device));
    }

    reportRejectedDevices(candidates);

    const auto best = selectBestCandidate(candidates, config.preferredName);

    if (!best.has_value())
    {
      reportNoSuitableDevices(candidates);

      throw std::runtime_error("failed to find a suitable GPU!");
    }

    DeviceCandidate& candidate = candidates[*best];

    m_physicalDevice          = candidate.device;
    m_properties              = candidate.properties;
    m_memoryProperties        = candidate.memoryProperties;
    m_deviceName              = candidate.properties.deviceName.data();
    m_queueFamilyIndices      = candidate.queueFamilyIndices;
    m_swapChainSupportDetails = std::move(candidate.swapChainSupport);
    m_msaaSamples             = candidate.msaaSamples;
    m_deviceLocalMemorySize   = candidate.deviceLocalMemorySize;
    m_supportsRayTracing      = candidate.supportsRayTracing;

    // Driver name and info are report-only, so they are queried once for the winner rather
    // than for every candidate. Copy the strings out: the struct's pNext points into the
    // chain this local owns, so keeping the struct itself would dangle.
    const auto propertyChain = m_physicalDevice.getProperties2<
      vk::PhysicalDeviceProperties2,
      vk::PhysicalDeviceDriverProperties
    >();

    const auto& driverProperties = propertyChain.get<vk::PhysicalDeviceDriverProperties>();
    m_driverName = driverProperties.driverName.data();
    m_driverInfo = driverProperties.driverInfo.data();
  }

  DeviceCandidate PhysicalDevice::evaluateDevice(const vk::raii::PhysicalDevice& device) const
  {
    DeviceCandidate candidate {
      .device = device,
      .properties = device.getProperties()
    };

    // The API version gate has to come before any getFeatures2 call: a driver silently ignores
    // pNext structs it does not know, leaving VulkanNNFeatures zeroed, so querying them on a
    // pre-1.3 device reports a bogus missing feature instead of the real reason.
    if (candidate.properties.apiVersion < vk::ApiVersion13)
    {
      candidate.rejectionReason = "reports Vulkan " +
        std::to_string(vk::apiVersionMajor(candidate.properties.apiVersion)) + "." +
        std::to_string(vk::apiVersionMinor(candidate.properties.apiVersion)) +
        ", engine requires 1.3";

      return candidate;
    }

    const auto availableExtensions = device.enumerateDeviceExtensionProperties();

    if (const auto missingExtension = findMissingExtension(availableExtensions, deviceExtensions);
        !missingExtension.empty())
    {
      candidate.rejectionReason = "missing device extension '" + std::string(missingExtension) + "'";

      return candidate;
    }

    candidate.supportsRayTracing = findMissingExtension(availableExtensions, rayTracingDeviceExtensions).empty();

    const auto featureChain = device.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan12Features,
      vk::PhysicalDeviceVulkan13Features
    >();

    const auto& vulkan12Features = featureChain.get<vk::PhysicalDeviceVulkan12Features>();

    for (const auto missingFeature : {
           findMissingFeature(featureChain.get<vk::PhysicalDeviceFeatures2>().features, requiredCoreFeatures),
           findMissingFeature(featureChain.get<vk::PhysicalDeviceVulkan11Features>(), requiredVulkan11Features),
           findMissingFeature(vulkan12Features, requiredVulkan12Features),
           findMissingFeature(featureChain.get<vk::PhysicalDeviceVulkan13Features>(), requiredVulkan13Features)
         })
    {
      if (!missingFeature.empty())
      {
        candidate.rejectionReason = "missing device feature '" + std::string(missingFeature) + "'";

        return candidate;
      }
    }

    if (candidate.supportsRayTracing)
    {
      // These are only requested when ray tracing is on, so missing them costs ray tracing
      // rather than making the device unsuitable. Querying the ray tracing feature structs is
      // only meaningful now that the extensions are known to be present.
      const auto rayTracingChain = device.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
      >();

      const bool rayTracingFeaturesSupported =
        findMissingFeature(vulkan12Features, rayTracingVulkan12Features).empty() &&
        findMissingFeature(rayTracingChain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>(),
                           rayTracingAccelerationStructureFeatures).empty() &&
        findMissingFeature(rayTracingChain.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>(),
                           rayTracingPipelineFeatures).empty();

      candidate.supportsRayTracing = rayTracingFeaturesSupported;
    }

    candidate.queueFamilyIndices = findQueueFamilies(device);

    if (!candidate.queueFamilyIndices.isComplete())
    {
      candidate.rejectionReason = "no queue families covering graphics, present and compute";

      return candidate;
    }

    candidate.swapChainSupport = querySwapChainSupport(device);

    if (candidate.swapChainSupport.formats.empty() || candidate.swapChainSupport.presentModes.empty())
    {
      candidate.rejectionReason = "no usable surface formats or present modes";

      return candidate;
    }

    candidate.memoryProperties = device.getMemoryProperties();
    candidate.deviceLocalMemorySize = findLargestDeviceLocalHeap(candidate.memoryProperties);
    candidate.msaaSamples = getMaxUsableSampleCount(candidate.properties.limits);
    candidate.score = scoreDevice(candidate);

    return candidate;
  }

  uint32_t PhysicalDevice::scoreDevice(const DeviceCandidate& candidate)
  {
    const auto heapScore = static_cast<uint32_t>(std::min(
      candidate.deviceLocalMemorySize / bytesPerMib / heapScoreMibPerPoint,
      static_cast<vk::DeviceSize>(heapScoreMax)));

    const auto msaaScore = std::min(static_cast<uint32_t>(candidate.msaaSamples), msaaScoreMax);

    const auto apiScore = std::min(vk::apiVersionMinor(candidate.properties.apiVersion), apiMinorScoreMax);

    return deviceTypeScore(candidate.properties.deviceType) +
           (candidate.supportsRayTracing ? rayTracingScore : 0) +
           heapScore +
           msaaScore +
           apiScore;
  }

  uint32_t PhysicalDevice::deviceTypeScore(const vk::PhysicalDeviceType deviceType)
  {
    switch (deviceType)
    {
      case vk::PhysicalDeviceType::eDiscreteGpu:   return deviceTypeScoreDiscreteGpu;
      case vk::PhysicalDeviceType::eIntegratedGpu: return deviceTypeScoreIntegratedGpu;
      case vk::PhysicalDeviceType::eVirtualGpu:    return deviceTypeScoreVirtualGpu;
      case vk::PhysicalDeviceType::eCpu:           return deviceTypeScoreCpu;
      default:                                     return deviceTypeScoreOther;
    }
  }

  std::optional<std::size_t> PhysicalDevice::selectBestCandidate(const std::vector<DeviceCandidate>& candidates,
                                                                const std::string_view preferredName)
  {
    std::optional<std::size_t> best;
    std::optional<std::size_t> bestPreferred;

    for (std::size_t i = 0; i < candidates.size(); i++)
    {
      const auto& candidate = candidates[i];

      if (!candidate.isSuitable())
      {
        continue;
      }

      if (!best.has_value() || candidate.score > candidates[*best].score)
      {
        best = i;
      }

      if (matchesPreferredName(candidate.properties.deviceName, preferredName) &&
          (!bestPreferred.has_value() || candidate.score > candidates[*bestPreferred].score))
      {
        bestPreferred = i;
      }
    }

    if (bestPreferred.has_value())
    {
      return bestPreferred;
    }

    // Silently ignoring an explicit override is a debugging trap, so say so and fall back.
    if (!preferredName.empty() && best.has_value())
    {
      std::cerr << "[VulkanEngine] no suitable GPU matches preferred name '" << preferredName
                << "', selecting automatically instead" << std::endl;
    }

    return best;
  }

  bool PhysicalDevice::matchesPreferredName(const std::string_view deviceName,
                                            const std::string_view preferredName)
  {
    if (preferredName.empty())
    {
      return false;
    }

    const auto found = std::ranges::search(deviceName, preferredName,
      [](const char left, const char right)
      {
        return std::tolower(static_cast<unsigned char>(left)) == std::tolower(static_cast<unsigned char>(right));
      });

    return !found.empty();
  }

  vk::DeviceSize PhysicalDevice::findLargestDeviceLocalHeap(const vk::PhysicalDeviceMemoryProperties& memoryProperties)
  {
    vk::DeviceSize largestHeap = 0;

    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
    {
      if (memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
      {
        largestHeap = std::max(largestHeap, memoryProperties.memoryHeaps[i].size);
      }
    }

    return largestHeap;
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

  SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(const vk::raii::PhysicalDevice& device) const
  {
    const auto surface = m_surface->getSurface();

    return {
      .capabilities = device.getSurfaceCapabilitiesKHR(surface),
      .formats = device.getSurfaceFormatsKHR(surface),
      .presentModes = device.getSurfacePresentModesKHR(surface)
    };
  }

  vk::SampleCountFlagBits PhysicalDevice::getMaxUsableSampleCount(const vk::PhysicalDeviceLimits& limits)
  {
    const vk::SampleCountFlags counts = limits.framebufferColorSampleCounts &
                                        limits.framebufferDepthSampleCounts;

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

  void PhysicalDevice::reportSelectedDevice() const
  {
    std::cout << "[VulkanEngine] selected GPU: " << m_deviceName
              << " (" << vk::to_string(m_properties.deviceType) << ")\n"
              << "                 device-local memory: " << m_deviceLocalMemorySize / bytesPerMib << " MiB"
              << " | max MSAA: " << vk::to_string(m_msaaSamples)
              << " | ray tracing: " << (m_supportsRayTracing ? "supported" : "unsupported") << "\n"
              << "                 Vulkan " << vk::apiVersionMajor(m_properties.apiVersion)
              << "." << vk::apiVersionMinor(m_properties.apiVersion)
              << "." << vk::apiVersionPatch(m_properties.apiVersion)
              << " | driver: " << m_driverName << " " << m_driverInfo << std::endl;
  }

  void PhysicalDevice::reportRejectedDevices(const std::vector<DeviceCandidate>& candidates)
  {
    #ifndef NDEBUG
    for (const auto& candidate : candidates)
    {
      if (candidate.isSuitable())
      {
        std::cout << "[VulkanEngine] considered GPU '" << candidate.properties.deviceName
                  << "': score " << candidate.score << std::endl;
      }
      else
      {
        std::cout << "[VulkanEngine] skipped GPU '" << candidate.properties.deviceName
                  << "': " << candidate.rejectionReason << std::endl;
      }
    }
    #endif
  }

  void PhysicalDevice::reportNoSuitableDevices(const std::vector<DeviceCandidate>& candidates)
  {
    // Unlike reportRejectedDevices this is not debug-only: when nothing is usable the engine is
    // about to throw, and these reasons are the only diagnostic the user gets.
    std::cerr << "[VulkanEngine] no suitable GPU found among " << candidates.size() << " device(s):" << std::endl;

    for (const auto& candidate : candidates)
    {
      std::cerr << "[VulkanEngine]   '" << candidate.properties.deviceName
                << "': " << candidate.rejectionReason << std::endl;
    }
  }

} // namespace vke
