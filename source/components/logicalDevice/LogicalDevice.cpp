#include "LogicalDevice.h"
#include "../instance/Instance.h"
#include "../physicalDevice/DeviceRequirements.h"
#include "../physicalDevice/PhysicalDevice.h"
#include <array>
#include <set>

namespace vke {

  LogicalDevice::LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
    : m_physicalDevice(physicalDevice)
  {
    createDevice();
  }

  std::shared_ptr<PhysicalDevice> LogicalDevice::getPhysicalDevice() const
  {
    return m_physicalDevice;
  }

  void LogicalDevice::waitIdle() const
  {
    m_device.waitIdle();
  }

  vk::Queue LogicalDevice::getGraphicsQueue() const
  {
    return *m_graphicsQueue;
  }

  vk::Queue LogicalDevice::getPresentQueue() const
  {
    return *m_presentQueue;
  }

  vk::Queue LogicalDevice::getComputeQueue() const
  {
    return *m_computeQueue;
  }

  void LogicalDevice::submitToGraphicsQueue(const vk::SubmitInfo2& submitInfo) const
  {
    m_graphicsQueue.submit2(submitInfo);
  }

  void LogicalDevice::submitToComputeQueue(const vk::SubmitInfo2& submitInfo) const
  {
    m_computeQueue.submit2(submitInfo);
  }

  vk::Result LogicalDevice::queuePresent(const vk::PresentInfoKHR& presentInfo) const
  {
    // eErrorOutOfDateKHR is returned (not thrown) thanks to
    // VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS, so callers can recreate the swapchain.
    return m_presentQueue.presentKHR(presentInfo);
  }

  std::pair<vk::Result, uint32_t> LogicalDevice::acquireNextImage(const vk::AcquireNextImageInfoKHR& acquireInfo) const
  {
    auto [result, imageIndex] = m_device.acquireNextImage2KHR(acquireInfo);

    return { result, imageIndex };
  }

  vk::raii::Semaphore LogicalDevice::createSemaphore(const vk::SemaphoreCreateInfo& semaphoreCreateInfo) const
  {
    return m_device.createSemaphore(semaphoreCreateInfo);
  }

  void LogicalDevice::waitSemaphores(const vk::SemaphoreWaitInfo& waitInfo) const
  {
    const auto result = m_device.waitSemaphores(waitInfo, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::signalSemaphore(const vk::SemaphoreSignalInfo& signalInfo) const
  {
    m_device.signalSemaphore(signalInfo);
  }

  uint32_t LogicalDevice::getMaxFramesInFlight() const
  {
    return m_maxFramesInFlight;
  }

  vk::raii::CommandPool LogicalDevice::createCommandPool(const vk::CommandPoolCreateInfo& commandPoolCreateInfo) const
  {
    return m_device.createCommandPool(commandPoolCreateInfo);
  }

  std::vector<vk::raii::DescriptorSet> LogicalDevice::allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& descriptorSetAllocateInfo) const
  {
    return m_device.allocateDescriptorSets(descriptorSetAllocateInfo);
  }

  void LogicalDevice::updateDescriptorSets(const std::vector<vk::WriteDescriptorSet>& writeDescriptorSets) const
  {
    m_device.updateDescriptorSets(writeDescriptorSets, nullptr);
  }

  vk::raii::Buffer LogicalDevice::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo) const
  {
    return m_device.createBuffer(bufferCreateInfo);
  }

  void LogicalDevice::allocateMemory(const vk::MemoryAllocateInfo& memoryAllocateInfo,
                                     vk::raii::DeviceMemory& deviceMemory) const
  {
    deviceMemory = m_device.allocateMemory(memoryAllocateInfo);
  }

  vk::raii::Sampler LogicalDevice::createSampler(const vk::SamplerCreateInfo& samplerCreateInfo) const
  {
    return m_device.createSampler(samplerCreateInfo);
  }

  vk::raii::ImageView LogicalDevice::createImageView(const vk::ImageViewCreateInfo& imageViewCreateInfo) const
  {
    return m_device.createImageView(imageViewCreateInfo);
  }

  vk::raii::Image LogicalDevice::createImage(const vk::ImageCreateInfo& imageCreateInfo) const
  {
    return m_device.createImage(imageCreateInfo);
  }

  vk::raii::ShaderModule LogicalDevice::createShaderModule(const vk::ShaderModuleCreateInfo& shaderModuleCreateInfo) const
  {
    return m_device.createShaderModule(shaderModuleCreateInfo);
  }

  vk::raii::SwapchainKHR LogicalDevice::createSwapchain(const vk::SwapchainCreateInfoKHR& swapchainCreateInfo) const
  {
    return m_device.createSwapchainKHR(swapchainCreateInfo);
  }

  vk::raii::Framebuffer LogicalDevice::createFramebuffer(const vk::FramebufferCreateInfo& framebufferCreateInfo) const
  {
    return m_device.createFramebuffer(framebufferCreateInfo);
  }

  vk::raii::PipelineLayout LogicalDevice::createPipelineLayout(const vk::PipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const
  {
    return m_device.createPipelineLayout(pipelineLayoutCreateInfo);
  }

  vk::raii::Pipeline LogicalDevice::createPipeline(const vk::GraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const
  {
    return m_device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo, nullptr);
  }

  vk::raii::Pipeline LogicalDevice::createPipeline(const vk::ComputePipelineCreateInfo& computePipelineCreateInfo) const
  {
    return m_device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
  }

  vk::raii::Pipeline LogicalDevice::createPipeline(const vk::RayTracingPipelineCreateInfoKHR& rayTracingPipelineCreateInfo) const
  {
    return m_device.createRayTracingPipelineKHR(nullptr, nullptr, rayTracingPipelineCreateInfo, nullptr);
  }

  vk::DeviceAddress LogicalDevice::getBufferDeviceAddress(const vk::Buffer buffer) const
  {
    const vk::BufferDeviceAddressInfo bufferDeviceAddressInfo {
      .buffer = buffer
    };

    return m_device.getBufferAddress(bufferDeviceAddressInfo);
  }

  vk::raii::AccelerationStructureKHR LogicalDevice::createAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& accelerationStructureCreateInfo) const
  {
    return m_device.createAccelerationStructureKHR(accelerationStructureCreateInfo, nullptr);
  }

  void LogicalDevice::getAccelerationStructureBuildSizes(const vk::AccelerationStructureBuildGeometryInfoKHR& accelerationStructureBuildGeometryInfo,
                                                         const uint32_t maxPrimitiveCounts,
                                                         vk::AccelerationStructureBuildSizesInfoKHR& accelerationStructureBuildSizesInfo) const
  {
    accelerationStructureBuildSizesInfo = m_device.getAccelerationStructureBuildSizesKHR(
      vk::AccelerationStructureBuildTypeKHR::eDevice,
      accelerationStructureBuildGeometryInfo,
      maxPrimitiveCounts
    );
  }

  vk::DeviceAddress LogicalDevice::getAccelerationStructureDeviceAddress(const vk::AccelerationStructureDeviceAddressInfoKHR* accelerationStructureDeviceAddressInfo) const
  {
    return m_device.getAccelerationStructureAddressKHR(*accelerationStructureDeviceAddressInfo);
  }

  void LogicalDevice::allocateCommandBuffers(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
                                             std::vector<vk::raii::CommandBuffer>& commandBuffers) const
  {
    commandBuffers = m_device.allocateCommandBuffers(commandBufferAllocateInfo);
  }

  vk::raii::DescriptorPool LogicalDevice::createDescriptorPool(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo) const
  {
    return m_device.createDescriptorPool(descriptorPoolCreateInfo);
  }

  vk::raii::DescriptorSetLayout LogicalDevice::createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const
  {
    return m_device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void LogicalDevice::createDevice()
  {
    auto queueFamilyIndices = m_physicalDevice->getQueueFamilies();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set uniqueQueueFamilies = {
      queueFamilyIndices.graphicsFamily.value(),
      queueFamilyIndices.presentFamily.value(),
      queueFamilyIndices.computeFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      const vk::DeviceQueueCreateInfo queueCreateInfo {
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
      };

      queueCreateInfos.push_back(queueCreateInfo);
    }

    // Every feature bit below comes from the lists in DeviceRequirements.h, which
    // PhysicalDevice also checks during selection. Adding a feature here directly would make
    // the engine request something selection never verified.
    const bool rayTracing = m_physicalDevice->supportsRayTracing();

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeaturesToRequest {};
    requestFeatures(rayTracingPipelineFeaturesToRequest, rayTracingPipelineFeatures);

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeaturesToRequest {
      .pNext = &rayTracingPipelineFeaturesToRequest
    };
    requestFeatures(accelerationStructureFeaturesToRequest, rayTracingAccelerationStructureFeatures);

    vk::PhysicalDeviceVulkan13Features vulkan13Features {
      .pNext = rayTracing ? &accelerationStructureFeaturesToRequest : nullptr
    };
    requestFeatures(vulkan13Features, requiredVulkan13Features);

    vk::PhysicalDeviceVulkan12Features vulkan12Features {
      .pNext = &vulkan13Features
    };
    requestFeatures(vulkan12Features, requiredVulkan12Features);

    if (rayTracing)
    {
      requestFeatures(vulkan12Features, rayTracingVulkan12Features);
    }

    vk::PhysicalDeviceVulkan11Features vulkan11Features {
      .pNext = &vulkan12Features
    };
    requestFeatures(vulkan11Features, requiredVulkan11Features);

    vk::PhysicalDeviceFeatures2 deviceFeatures2 {
      .pNext = &vulkan11Features
    };
    requestFeatures(deviceFeatures2.features, requiredCoreFeatures);

    auto extensions = std::vector(deviceExtensions.begin(), deviceExtensions.end());

    if (rayTracing)
    {
      extensions.insert(extensions.end(), rayTracingDeviceExtensions.begin(), rayTracingDeviceExtensions.end());
    }

    const vk::DeviceCreateInfo createInfo {
      .pNext = &deviceFeatures2,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()
    };

    m_device = m_physicalDevice->createLogicalDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

    m_computeQueue = m_device.getQueue(queueFamilyIndices.computeFamily.value(), 0);
    m_graphicsQueue = m_device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(queueFamilyIndices.presentFamily.value(), 0);
  }

} // namespace vke