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

  vk::raii::Queue LogicalDevice::getGraphicsQueue() const
  {
    return m_graphicsQueue;
  }

  vk::raii::Queue LogicalDevice::getPresentQueue() const
  {
    return m_presentQueue;
  }

  vk::raii::Queue LogicalDevice::getComputeQueue() const
  {
    return m_computeQueue;
  }

  void LogicalDevice::submitMousePickingGraphicsQueue(const uint32_t currentFrame,
                                                      const vk::raii::CommandBuffer* commandBuffer) const
  {
    constexpr vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eVertexInput,
      vk::PipelineStageFlagBits::eColorAttachmentOutput
    };

    const vk::SubmitInfo submitInfo {
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &**commandBuffer,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };

    m_graphicsQueue.submit(submitInfo, m_mousePickingInFlightFences[currentFrame]);
  }

  void LogicalDevice::submitOffscreenGraphicsQueue(const uint32_t currentFrame,
                                                   const vk::raii::CommandBuffer* commandBuffer) const
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
      .pCommandBuffers = &**commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_renderFinishedSemaphores2[currentFrame]
    };

    m_graphicsQueue.submit(submitInfo, m_offscreenInFlightFences[currentFrame]);
  }

  void LogicalDevice::submitGraphicsQueue(const uint32_t currentFrame,
                                          const vk::raii::CommandBuffer* commandBuffer) const
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
      .pCommandBuffers = &**commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_renderFinishedSemaphores[currentFrame]
    };

    m_graphicsQueue.submit(submitInfo, m_inFlightFences[currentFrame]);
  }

  void LogicalDevice::submitComputeQueue(const uint32_t currentFrame,
                                         const vk::raii::CommandBuffer* commandBuffer) const
  {
    const vk::SubmitInfo submitInfo {
      .commandBufferCount = 1,
      .pCommandBuffers = &**commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*m_computeFinishedSemaphores[currentFrame]
    };

    m_computeQueue.submit(submitInfo, m_computeInFlightFences[currentFrame]);
  }

  void LogicalDevice::waitForGraphicsFences(const uint32_t currentFrame) const
  {
    const std::array fences = {
      *m_inFlightFences[currentFrame],
      *m_offscreenInFlightFences[currentFrame]
    };

    const auto result = m_device.waitForFences(fences, VK_TRUE, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::waitForComputeFences(const uint32_t currentFrame) const
  {
    const auto result = m_device.waitForFences(*m_computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
  }

  void LogicalDevice::waitForMousePickingFences(const uint32_t currentFrame) const
  {
    const auto result = m_device.waitForFences(*m_mousePickingInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
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

  void LogicalDevice::resetMousePickingFences(const uint32_t currentFrame) const
  {
    m_device.resetFences(*m_mousePickingInFlightFences[currentFrame]);
  }

  void LogicalDevice::resetComputeFences(const uint32_t currentFrame) const
  {
    m_device.resetFences(*m_computeInFlightFences[currentFrame]);
  }

  vk::Result LogicalDevice::queuePresent(const uint32_t currentFrame,
                                       const vk::raii::SwapchainKHR& swapchain,
                                       const uint32_t* imageIndex) const
  {
    const std::array waitSemaphores = {
      *m_renderFinishedSemaphores[currentFrame],
      *m_renderFinishedSemaphores2[currentFrame]
    };

    const vk::PresentInfoKHR presentInfo {
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &*swapchain,
      .pImageIndices = imageIndex,
      .pResults = nullptr
    };

    return m_presentQueue.presentKHR(presentInfo);
  }

  void LogicalDevice::acquireNextImage(const uint32_t currentFrame,
                                       const vk::raii::SwapchainKHR& swapchain,
                                       uint32_t* imageIndex) const
  {
    const vk::AcquireNextImageInfoKHR acquireInfo {
      .swapchain = swapchain,
      .timeout = UINT64_MAX,
      .semaphore = *m_imageAvailableSemaphores[currentFrame],
      .fence = VK_NULL_HANDLE,
      .deviceMask = 0
    };

    auto [result, index] = m_device.acquireNextImage2KHR(acquireInfo);
    *imageIndex = index;
  }

  uint32_t LogicalDevice::getMaxFramesInFlight() const
  {
    return m_maxFramesInFlight;
  }

  vk::raii::CommandPool LogicalDevice::createCommandPool(const vk::CommandPoolCreateInfo& commandPoolCreateInfo) const
  {
    return m_device.createCommandPool(commandPoolCreateInfo);
  }

  void LogicalDevice::doMappedMemoryOperation(const vk::raii::DeviceMemory& deviceMemory,
                                              const std::function<void(void* data)>& operationFunction)
  {
    void* data = deviceMemory.mapMemory(0, VK_WHOLE_SIZE);

    operationFunction(data);

    deviceMemory.unmapMemory();
  }

  void LogicalDevice::mapMemory(const vk::raii::DeviceMemory& memory,
                                const vk::DeviceSize offset,
                                const vk::DeviceSize size,
                                const vk::MemoryMapFlags flags,
                                void** data)
  {
    *data = memory.mapMemory(offset, size, flags);
  }

  void LogicalDevice::unmapMemory(const vk::raii::DeviceMemory& memory)
  {
    if (*memory == VK_NULL_HANDLE)
    {
      return;
    }

    memory.unmapMemory();
  }

  void LogicalDevice::allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& descriptorSetAllocateInfo,
                                             vk::raii::DescriptorSet* descriptorSets) const
  {
    auto sets = m_device.allocateDescriptorSets(descriptorSetAllocateInfo);
    std::ranges::move(sets, descriptorSets);
  }

  void LogicalDevice::updateDescriptorSets(const vk::WriteDescriptorSet& descriptorWrites) const
  {
    m_device.updateDescriptorSets(descriptorWrites, nullptr);
  }

  vk::raii::Buffer LogicalDevice::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo) const
  {
    return m_device.createBuffer(bufferCreateInfo);
  }

  vk::MemoryRequirements LogicalDevice::getBufferMemoryRequirements(const vk::raii::Buffer& buffer)
  {
    return buffer.getMemoryRequirements();
  }

  void LogicalDevice::allocateMemory(const vk::MemoryAllocateInfo& memoryAllocateInfo,
                                     vk::raii::DeviceMemory& deviceMemory) const
  {
    deviceMemory = m_device.allocateMemory(memoryAllocateInfo);
  }

  void LogicalDevice::bindBufferMemory(const vk::raii::Buffer& buffer,
                                       const vk::raii::DeviceMemory& deviceMemory,
                                       const vk::DeviceSize memoryOffset)
  {
    buffer.bindMemory(*deviceMemory, memoryOffset);
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

  vk::MemoryRequirements LogicalDevice::getImageMemoryRequirements(const vk::raii::Image& image)
  {
    return image.getMemoryRequirements();
  }

  void LogicalDevice::bindImageMemory(const vk::raii::Image& image,
                                      const vk::raii::DeviceMemory& deviceMemory,
                                      const vk::DeviceSize memoryOffset)
  {
    image.bindMemory(*deviceMemory, memoryOffset);
  }

  vk::raii::RenderPass LogicalDevice::createRenderPass(const vk::RenderPassCreateInfo& renderPassCreateInfo) const
  {
    return m_device.createRenderPass(renderPassCreateInfo);
  }

  vk::raii::ShaderModule LogicalDevice::createShaderModule(const vk::ShaderModuleCreateInfo& shaderModuleCreateInfo) const
  {
    return m_device.createShaderModule(shaderModuleCreateInfo);
  }

  vk::raii::SwapchainKHR LogicalDevice::createSwapchain(const vk::SwapchainCreateInfoKHR& swapchainCreateInfo) const
  {
    return m_device.createSwapchainKHR(swapchainCreateInfo);
  }

  void LogicalDevice::getSwapchainImagesKHR(const vk::raii::SwapchainKHR& swapchain,
                                            uint32_t* swapchainImageCount,
                                            vk::Image* swapchainImages)
  {
    auto images = swapchain.getImages();
    *swapchainImageCount = static_cast<uint32_t>(images.size());
    std::ranges::copy(images, swapchainImages);
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
    return m_device.createGraphicsPipeline(VK_NULL_HANDLE, graphicsPipelineCreateInfo, nullptr);
  }

  vk::raii::Pipeline LogicalDevice::createPipeline(const vk::ComputePipelineCreateInfo& computePipelineCreateInfo) const
  {
    return m_device.createComputePipeline(VK_NULL_HANDLE, computePipelineCreateInfo, nullptr);
  }

  vk::raii::Pipeline LogicalDevice::createPipeline(const vk::RayTracingPipelineCreateInfoKHR& rayTracingPipelineCreateInfo) const
  {
    return m_device.createRayTracingPipelineKHR(VK_NULL_HANDLE, VK_NULL_HANDLE, rayTracingPipelineCreateInfo, nullptr);
  }

  vk::DeviceAddress LogicalDevice::getBufferDeviceAddress(const vk::raii::Buffer& buffer) const
  {
    const vk::BufferDeviceAddressInfo bufferDeviceAddressInfo {
      .buffer = buffer
    };

    return m_device.getBufferAddress(bufferDeviceAddressInfo);
  }

  void LogicalDevice::createAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& accelerationStructureCreateInfo,
                                                  vk::raii::AccelerationStructureKHR* accelerationStructure) const
  {
    *accelerationStructure = m_device.createAccelerationStructureKHR(accelerationStructureCreateInfo, nullptr);
  }

  void LogicalDevice::getAccelerationStructureBuildSizes(const vk::AccelerationStructureBuildGeometryInfoKHR& accelerationStructureBuildGeometryInfo,
                                                         const uint32_t maxPrimitiveCounts,
                                                         vk::AccelerationStructureBuildSizesInfoKHR* accelerationStructureBuildSizesInfo) const
  {
    *accelerationStructureBuildSizesInfo = m_device.getAccelerationStructureBuildSizesKHR(
      vk::AccelerationStructureBuildTypeKHR::eDevice,
      accelerationStructureBuildGeometryInfo,
      maxPrimitiveCounts
    );
  }

  void LogicalDevice::buildAccelerationStructures(const vk::raii::CommandBuffer& commandBuffer,
                                                  const vk::AccelerationStructureBuildGeometryInfoKHR& pInfos,
                                                  const vk::AccelerationStructureBuildRangeInfoKHR* ppBuildRangeInfos)
  {
    commandBuffer.buildAccelerationStructuresKHR(
      pInfos,
      ppBuildRangeInfos
    );
  }

  vk::DeviceAddress LogicalDevice::getAccelerationStructureDeviceAddress(const vk::AccelerationStructureDeviceAddressInfoKHR* accelerationStructureDeviceAddressInfo) const
  {
    return m_device.getAccelerationStructureAddressKHR(*accelerationStructureDeviceAddressInfo);
  }

  void LogicalDevice::getRayTracingShaderGroupHandles(const vk::raii::Pipeline& pipeline,
                                                      const uint32_t groupCount,
                                                      std::vector<uint8_t>& handles)
  {
    handles = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(
      0,
      groupCount,
      handles.size()
    );
  }

  void LogicalDevice::allocateCommandBuffers(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
                                             std::vector<vk::raii::CommandBuffer>& commandBuffers) const
  {
    auto buffers = m_device.allocateCommandBuffers(commandBufferAllocateInfo);
    std::ranges::copy(buffers, commandBuffers);
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
      .rayTracingPipeline = VK_TRUE
    };

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures {
      .pNext = &rayTracingPipelineFeatures,
      .accelerationStructure = VK_TRUE
    };

    vk::PhysicalDeviceVulkan13Features vulkan13Features {
      .pNext = getPhysicalDevice()->supportsRayTracing() ? &accelerationStructureFeatures : nullptr,
      .dynamicRendering = VK_TRUE
    };

    vk::PhysicalDeviceVulkan12Features vulkan12Features {
      .pNext = &vulkan13Features,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      .descriptorBindingPartiallyBound = VK_TRUE,
      .descriptorBindingVariableDescriptorCount = getPhysicalDevice()->supportsRayTracing() ? VK_TRUE : VK_FALSE,
      .runtimeDescriptorArray = VK_TRUE,
      .bufferDeviceAddress = getPhysicalDevice()->supportsRayTracing() ? VK_TRUE : VK_FALSE
    };

    vk::PhysicalDeviceVulkan11Features vulkan11Features {
      .pNext = &vulkan12Features,
      .multiview = VK_TRUE
    };

    vk::PhysicalDeviceFeatures2 deviceFeatures2 {
      .pNext = &vulkan11Features,
      .features {
        .geometryShader = VK_TRUE,
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
      }
    };

    auto extensions = std::vector<const char*>(deviceExtensions.begin(), deviceExtensions.end());

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

    m_computeQueue = m_device.getQueue(queueFamilyIndices.computeFamily.value(), 0);
    m_graphicsQueue = m_device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(queueFamilyIndices.presentFamily.value(), 0);
  }

  void LogicalDevice::createSyncObjects()
  {
    m_imageAvailableSemaphores.resize(m_maxFramesInFlight);

    m_renderFinishedSemaphores.resize(m_maxFramesInFlight);
    m_renderFinishedSemaphores2.resize(m_maxFramesInFlight);

    m_computeFinishedSemaphores.resize(m_maxFramesInFlight);

    m_inFlightFences.resize(m_maxFramesInFlight);
    m_offscreenInFlightFences.resize(m_maxFramesInFlight);
    m_mousePickingInFlightFences.resize(m_maxFramesInFlight);
    m_computeInFlightFences.resize(m_maxFramesInFlight);

    constexpr vk::FenceCreateInfo fenceInfo {
      .flags = vk::FenceCreateFlagBits::eSignaled
    };

    for (size_t i = 0; i < m_maxFramesInFlight; i++)
    {
      constexpr vk::SemaphoreCreateInfo semaphoreInfo {};

      m_imageAvailableSemaphores[i] = m_device.createSemaphore(semaphoreInfo);
      m_renderFinishedSemaphores[i] = m_device.createSemaphore(semaphoreInfo);
      m_renderFinishedSemaphores2[i] = m_device.createSemaphore(semaphoreInfo);
      m_inFlightFences[i] = m_device.createFence(fenceInfo);
      m_offscreenInFlightFences[i] = m_device.createFence(fenceInfo);
      m_mousePickingInFlightFences[i] = m_device.createFence(fenceInfo);

      m_computeFinishedSemaphores[i] = m_device.createSemaphore(semaphoreInfo);
      m_computeInFlightFences[i] = m_device.createFence(fenceInfo);
    }
  }
} // namespace vke