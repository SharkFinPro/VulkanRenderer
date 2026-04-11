#include "LogicalDevice.h"
#include "../instance/Instance.h"
#include "../physicalDevice/PhysicalDevice.h"
#include <array>
#include <set>

namespace vke {

  LogicalDevice::LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
    : m_physicalDevice(physicalDevice)
  {
    createDevice();

    createSyncObjects();
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

  void LogicalDevice::submitOffscreenCommandBuffer(const uint32_t currentFrame,
                                                   const vk::CommandBuffer commandBuffer) const
  {
    constexpr vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eVertexInput,
      vk::PipelineStageFlagBits::eColorAttachmentOutput
    };

    const vk::SubmitInfo submitInfo {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*m_computeFinishedSemaphores[currentFrame],
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_offscreenRenderFinishedSemaphores[currentFrame]
    };

    m_graphicsQueue.submit(submitInfo, m_offscreenInFlightFences[currentFrame]);
  }

  void LogicalDevice::submitSwapchainCommandBuffer(const uint32_t currentFrame,
                                                   const vk::CommandBuffer commandBuffer) const
  {
    constexpr vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eVertexInput,
      vk::PipelineStageFlagBits::eColorAttachmentOutput
    };

    const vk::SubmitInfo submitInfo {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*m_imageAvailableSemaphores[currentFrame],
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_renderFinishedSemaphores[currentFrame]
    };

    m_graphicsQueue.submit(submitInfo, m_inFlightFences[currentFrame]);
  }

  void LogicalDevice::submitComputeCommandBuffer(const uint32_t currentFrame,
                                                 const vk::CommandBuffer commandBuffer) const
  {
    const vk::SubmitInfo submitInfo {
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_computeFinishedSemaphores[currentFrame]
    };

    m_computeQueue.submit(submitInfo, m_computeInFlightFences[currentFrame]);
  }

  void LogicalDevice::waitForOffscreenFence(const uint32_t currentFrame) const
  {
    const auto result = m_device.waitForFences(*m_offscreenInFlightFences[currentFrame], vk::True, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::waitForGraphicsFences(const uint32_t currentFrame) const
  {
    const std::array fences = {
      *m_inFlightFences[currentFrame],
      *m_offscreenInFlightFences[currentFrame]
    };

    const auto result = m_device.waitForFences(fences, vk::True, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::waitForComputeFences(const uint32_t currentFrame) const
  {
    const auto result = m_device.waitForFences(*m_computeInFlightFences[currentFrame], vk::True, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::resetGraphicsFences(const uint32_t currentFrame) const
  {
    const std::array fences = {
      *m_inFlightFences[currentFrame],
      *m_offscreenInFlightFences[currentFrame]
    };

    m_device.resetFences(fences);
  }

  void LogicalDevice::resetComputeFences(const uint32_t currentFrame) const
  {
    m_device.resetFences(*m_computeInFlightFences[currentFrame]);
  }

  vk::Result LogicalDevice::queuePresent(const uint32_t currentFrame,
                                         const vk::SwapchainKHR swapchain,
                                         const uint32_t* imageIndex) const
  {
    const std::array waitSemaphores = {
      *m_renderFinishedSemaphores[currentFrame],
      *m_offscreenRenderFinishedSemaphores[currentFrame]
    };

    const vk::PresentInfoKHR presentInfo {
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = imageIndex,
      .pResults = nullptr
    };

    return m_presentQueue.presentKHR(presentInfo);
  }

  vk::Result LogicalDevice::acquireNextImage(const uint32_t currentFrame,
                                             const vk::SwapchainKHR swapchain,
                                             uint32_t* imageIndex) const
  {
    const vk::AcquireNextImageInfoKHR acquireInfo {
      .swapchain = swapchain,
      .timeout = UINT64_MAX,
      .semaphore = *m_imageAvailableSemaphores[currentFrame],
      .fence = nullptr,
      .deviceMask = 1
    };

    auto [result, index] = m_device.acquireNextImage2KHR(acquireInfo);
    *imageIndex = index;

    return result;
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
      queueFamilyIndices.presentFamily.value()
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

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures {
      .rayTracingPipeline = vk::True
    };

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures {
      .pNext = &rayTracingPipelineFeatures,
      .accelerationStructure = vk::True
    };

    vk::PhysicalDeviceVulkan13Features vulkan13Features {
      .pNext = getPhysicalDevice()->supportsRayTracing() ? &accelerationStructureFeatures : nullptr,
      .dynamicRendering = vk::True
    };

    vk::PhysicalDeviceVulkan12Features vulkan12Features {
      .pNext = &vulkan13Features,
      .shaderSampledImageArrayNonUniformIndexing = vk::True,
      .descriptorBindingPartiallyBound = vk::True,
      .descriptorBindingVariableDescriptorCount = getPhysicalDevice()->supportsRayTracing() ? vk::True : vk::False,
      .runtimeDescriptorArray = vk::True,
      .bufferDeviceAddress = getPhysicalDevice()->supportsRayTracing() ? vk::True : vk::False
    };

    vk::PhysicalDeviceVulkan11Features vulkan11Features {
      .pNext = &vulkan12Features,
      .multiview = vk::True
    };

    vk::PhysicalDeviceFeatures2 deviceFeatures2 {
      .pNext = &vulkan11Features,
      .features {
        .geometryShader = vk::True,
        .fillModeNonSolid = vk::True,
        .samplerAnisotropy = vk::True
      }
    };

    auto extensions = std::vector(deviceExtensions.begin(), deviceExtensions.end());

    if (m_physicalDevice->supportsRayTracing())
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

  void LogicalDevice::createSyncObjects()
  {
    m_imageAvailableSemaphores.reserve(m_maxFramesInFlight);

    m_renderFinishedSemaphores.reserve(m_maxFramesInFlight);
    m_offscreenRenderFinishedSemaphores.reserve(m_maxFramesInFlight);

    m_computeFinishedSemaphores.reserve(m_maxFramesInFlight);

    m_inFlightFences.reserve(m_maxFramesInFlight);
    m_offscreenInFlightFences.reserve(m_maxFramesInFlight);
    m_computeInFlightFences.reserve(m_maxFramesInFlight);

    constexpr vk::FenceCreateInfo fenceInfo {
      .flags = vk::FenceCreateFlagBits::eSignaled
    };

    for (size_t i = 0; i < m_maxFramesInFlight; i++)
    {
      constexpr vk::SemaphoreCreateInfo semaphoreInfo {};

      m_imageAvailableSemaphores.emplace_back(m_device.createSemaphore(semaphoreInfo));
      m_renderFinishedSemaphores.emplace_back(m_device.createSemaphore(semaphoreInfo));
      m_offscreenRenderFinishedSemaphores.emplace_back(m_device.createSemaphore(semaphoreInfo));
      m_inFlightFences.emplace_back(m_device.createFence(fenceInfo));
      m_offscreenInFlightFences.emplace_back(m_device.createFence(fenceInfo));

      m_computeFinishedSemaphores.emplace_back(m_device.createSemaphore(semaphoreInfo));
      m_computeInFlightFences.emplace_back(m_device.createFence(fenceInfo));
    }
  }
} // namespace vke