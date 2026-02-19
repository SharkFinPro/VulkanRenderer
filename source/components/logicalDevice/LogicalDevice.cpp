#include "LogicalDevice.h"
#include "../instance/Instance.h"
#include "../physicalDevice/PhysicalDevice.h"
#include <array>
#include <set>
#include <stdexcept>

namespace vke {

  LogicalDevice::LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
    : m_physicalDevice(physicalDevice)
  {
    createDevice();

    createSyncObjects();
  }

  LogicalDevice::~LogicalDevice()
  {
    for (size_t i = 0; i < m_maxFramesInFlight; i++)
    {
      vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(m_device, m_renderFinishedSemaphores2[i], nullptr);

      vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);

      vkDestroySemaphore(m_device, m_computeFinishedSemaphores[i], nullptr);

      vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
      vkDestroyFence(m_device, m_inFlightFences2[i], nullptr);
      vkDestroyFence(m_device, m_mousePickingInFlightFences[i], nullptr);
      vkDestroyFence(m_device, m_computeInFlightFences[i], nullptr);
    }

    vkDestroyDevice(m_device, nullptr);
  }

  std::shared_ptr<PhysicalDevice> LogicalDevice::getPhysicalDevice() const
  {
    return m_physicalDevice;
  }

  void LogicalDevice::waitIdle() const
  {
    vkDeviceWaitIdle(m_device);
  }

  VkQueue LogicalDevice::getGraphicsQueue() const
  {
    return m_graphicsQueue;
  }

  VkQueue LogicalDevice::getPresentQueue() const
  {
    return m_presentQueue;
  }

  VkQueue LogicalDevice::getComputeQueue() const
  {
    return m_computeQueue;
  }

  void LogicalDevice::submitMousePickingGraphicsQueue(const uint32_t currentFrame,
                                                      const VkCommandBuffer* commandBuffer) const
  {
    constexpr VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = commandBuffer,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_mousePickingInFlightFences[currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit draw command buffer!");
    }
  }

  void LogicalDevice::submitOffscreenGraphicsQueue(const uint32_t currentFrame,
                                                   const VkCommandBuffer* commandBuffer) const
  {
    const std::array<VkSemaphore, 1> waitSemaphores = {
      m_computeFinishedSemaphores[currentFrame]
    };
    constexpr VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_renderFinishedSemaphores2[currentFrame]
    };

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences2[currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit draw command buffer!");
    }
  }

  void LogicalDevice::submitGraphicsQueue(const uint32_t currentFrame,
                                          const VkCommandBuffer* commandBuffer) const
  {
    const std::array<VkSemaphore, 1> waitSemaphores = {
      m_imageAvailableSemaphores[currentFrame]
    };
    constexpr VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_renderFinishedSemaphores[currentFrame]
    };

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit draw command buffer!");
    }
  }

  void LogicalDevice::submitComputeQueue(const uint32_t currentFrame,
                                         const VkCommandBuffer* commandBuffer) const
  {
    const VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_computeFinishedSemaphores[currentFrame]
    };

    if (vkQueueSubmit(m_computeQueue, 1, &submitInfo, m_computeInFlightFences[currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit compute command buffer!");
    }
  }

  void LogicalDevice::waitForGraphicsFences(const uint32_t currentFrame) const
  {
    vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkWaitForFences(m_device, 1, &m_inFlightFences2[currentFrame], VK_TRUE, UINT64_MAX);
  }
  void LogicalDevice::waitForComputeFences(const uint32_t currentFrame) const
  {
    vkWaitForFences(m_device, 1, &m_computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  }

  void LogicalDevice::waitForMousePickingFences(const uint32_t currentFrame) const
  {
    vkWaitForFences(m_device, 1, &m_mousePickingInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  }

  void LogicalDevice::resetGraphicsFences(const uint32_t currentFrame) const
  {
    vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);
    vkResetFences(m_device, 1, &m_inFlightFences2[currentFrame]);
  }

  void LogicalDevice::resetMousePickingFences(const uint32_t currentFrame) const
  {
    vkResetFences(m_device, 1, &m_mousePickingInFlightFences[currentFrame]);
  }

  void LogicalDevice::resetComputeFences(const uint32_t currentFrame) const
  {
    vkResetFences(m_device, 1, &m_computeInFlightFences[currentFrame]);
  }

  VkResult LogicalDevice::queuePresent(const uint32_t currentFrame,
                                       const VkSwapchainKHR& swapchain,
                                       const uint32_t* imageIndex) const
  {
    const std::array<VkSemaphore, 2> waitSemaphores = {
      m_renderFinishedSemaphores[currentFrame],
      m_renderFinishedSemaphores2[currentFrame]
    };

    const VkPresentInfoKHR presentInfo {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = imageIndex,
      .pResults = nullptr
    };

    return vkQueuePresentKHR(m_presentQueue, &presentInfo);
  }

  VkResult LogicalDevice::acquireNextImage(const uint32_t currentFrame,
                                           const VkSwapchainKHR& swapchain,
                                           uint32_t* imageIndex) const
  {
    return vkAcquireNextImageKHR(m_device, swapchain, UINT64_MAX,
                                 m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
  }

  uint32_t LogicalDevice::getMaxFramesInFlight() const
  {
    return m_maxFramesInFlight;
  }

  VkCommandPool LogicalDevice::createCommandPool(const VkCommandPoolCreateInfo& commandPoolCreateInfo) const
  {
    VkCommandPool commandPool = VK_NULL_HANDLE;

    if (vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create command pool!");
    }

    return commandPool;
  }

  void LogicalDevice::destroyCommandPool(VkCommandPool& commandPool) const
  {
    vkDestroyCommandPool(m_device, commandPool, nullptr);

    commandPool = VK_NULL_HANDLE;
  }

  void LogicalDevice::destroyDescriptorPool(VkDescriptorPool& descriptorPool) const
  {
    vkDestroyDescriptorPool(m_device, descriptorPool, nullptr);

    descriptorPool = VK_NULL_HANDLE;
  }

  void LogicalDevice::destroyDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout) const
  {
    vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);

    descriptorSetLayout = VK_NULL_HANDLE;
  }

  void LogicalDevice::doMappedMemoryOperation(VkDeviceMemory deviceMemory,
                                              const std::function<void(void* data)>& operationFunction) const
  {
    void* data;
    mapMemory(deviceMemory, 0, VK_WHOLE_SIZE, 0, &data);

    operationFunction(data);

    unmapMemory(deviceMemory);
  }

  void LogicalDevice::mapMemory(const VkDeviceMemory& memory,
                                const VkDeviceSize offset,
                                const VkDeviceSize size,
                                const VkMemoryMapFlags flags,
                                void** data) const
  {
    vkMapMemory(m_device, memory, offset, size, flags, data);
  }

  void LogicalDevice::unmapMemory(const VkDeviceMemory& memory) const
  {
    if (memory == VK_NULL_HANDLE)
    {
      return;
    }

    vkUnmapMemory(m_device, memory);
  }

  void LogicalDevice::allocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo,
                                             VkDescriptorSet* descriptorSets) const
  {
    if (vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, descriptorSets) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }
  }

  void LogicalDevice::updateDescriptorSets(const uint32_t descriptorWriteCount,
                                           const VkWriteDescriptorSet* descriptorWrites) const
  {
    vkUpdateDescriptorSets(m_device, descriptorWriteCount, descriptorWrites, 0, nullptr);
  }

  VkBuffer LogicalDevice::createBuffer(const VkBufferCreateInfo& bufferCreateInfo) const
  {
    VkBuffer buffer = VK_NULL_HANDLE;

    if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create buffer!");
    }

    return buffer;
  }

  void LogicalDevice::destroyBuffer(VkBuffer&buffer) const
  {
    if (buffer == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroyBuffer(m_device, buffer, nullptr);

    buffer = VK_NULL_HANDLE;
  }

  VkMemoryRequirements LogicalDevice::getBufferMemoryRequirements(const VkBuffer& buffer) const
  {
    VkMemoryRequirements memoryRequirements{};

    vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);

    return memoryRequirements;
  }

  void LogicalDevice::allocateMemory(const VkMemoryAllocateInfo& memoryAllocateInfo,
                                     VkDeviceMemory& deviceMemory) const
  {
    if (vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate memory!");
    }
  }

  void LogicalDevice::freeMemory(VkDeviceMemory& memory) const
  {
    if (memory == VK_NULL_HANDLE)
    {
      return;
    }

    waitIdle();

    vkFreeMemory(m_device, memory, nullptr);

    memory = VK_NULL_HANDLE;
  }

  void LogicalDevice::bindBufferMemory(const VkBuffer& buffer,
                                       const VkDeviceMemory& deviceMemory,
                                       const VkDeviceSize memoryOffset) const
  {
    vkBindBufferMemory(m_device, buffer, deviceMemory, memoryOffset);
  }

  VkSampler LogicalDevice::createSampler(const VkSamplerCreateInfo& samplerCreateInfo) const
  {
    VkSampler sampler = VK_NULL_HANDLE;

    if (vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create sampler!");
    }

    return sampler;
  }

  void LogicalDevice::destroySampler(VkSampler& sampler) const
  {
    if (sampler == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroySampler(m_device, sampler, nullptr);

    sampler = VK_NULL_HANDLE;
  }

  VkImageView LogicalDevice::createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const
  {
    VkImageView imageView = VK_NULL_HANDLE;

    if (vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create image view!");
    }

    return imageView;
  }

  void LogicalDevice::destroyImageView(VkImageView& imageView) const
  {
    if (imageView == VK_NULL_HANDLE)
    {
      return;
    }

    waitIdle();

    vkDestroyImageView(m_device, imageView, nullptr);

    imageView = VK_NULL_HANDLE;
  }

  VkImage LogicalDevice::createImage(const VkImageCreateInfo& imageCreateInfo) const
  {
    VkImage image = VK_NULL_HANDLE;

    if (vkCreateImage(m_device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create image!");
    }

    return image;
  }

  void LogicalDevice::destroyImage(VkImage& image) const
  {
    if (image == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroyImage(m_device, image, nullptr);

    image = VK_NULL_HANDLE;
  }

  VkMemoryRequirements LogicalDevice::getImageMemoryRequirements(const VkImage& image) const
  {
    VkMemoryRequirements memoryRequirements{};

    vkGetImageMemoryRequirements(m_device, image, &memoryRequirements);

    return memoryRequirements;
  }

  void LogicalDevice::bindImageMemory(const VkImage& image,
                                      const VkDeviceMemory& deviceMemory,
                                      const VkDeviceSize memoryOffset) const
  {
    vkBindImageMemory(m_device, image, deviceMemory, memoryOffset);
  }

  VkRenderPass LogicalDevice::createRenderPass(const VkRenderPassCreateInfo& renderPassCreateInfo) const
  {
    VkRenderPass renderPass = VK_NULL_HANDLE;

    if (vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create render pass!");
    }

    return renderPass;
  }

  void LogicalDevice::destroyRenderPass(VkRenderPass& renderPass) const
  {
    vkDestroyRenderPass(m_device, renderPass, nullptr);

    renderPass = VK_NULL_HANDLE;
  }

  VkShaderModule LogicalDevice::createShaderModule(const VkShaderModuleCreateInfo& shaderModuleCreateInfo) const
  {
    VkShaderModule shaderModule = VK_NULL_HANDLE;

    if (vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
  }

  void LogicalDevice::destroyShaderModule(VkShaderModule& shaderModule) const
  {
    vkDestroyShaderModule(m_device, shaderModule, nullptr);

    shaderModule = VK_NULL_HANDLE;
  }

  VkSwapchainKHR LogicalDevice::createSwapchain(const VkSwapchainCreateInfoKHR& swapchainCreateInfo) const
  {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create swapchain!");
    }

    return swapchain;
  }

  void LogicalDevice::getSwapchainImagesKHR(const VkSwapchainKHR& swapchain,
                                            uint32_t* swapchainImageCount,
                                            VkImage* swapchainImages) const
  {
    vkGetSwapchainImagesKHR(m_device, swapchain, swapchainImageCount, swapchainImages);
  }

  void LogicalDevice::destroySwapchainKHR(VkSwapchainKHR& swapchain) const
  {
    vkDestroySwapchainKHR(m_device, swapchain, nullptr);

    swapchain = VK_NULL_HANDLE;
  }

  VkFramebuffer LogicalDevice::createFramebuffer(const VkFramebufferCreateInfo& framebufferCreateInfo) const
  {
    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    if (vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }

    return framebuffer;
  }

  void LogicalDevice::destroyFramebuffer(VkFramebuffer& framebuffer) const
  {
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);

    framebuffer = VK_NULL_HANDLE;
  }

  VkPipelineLayout LogicalDevice::createPipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const
  {
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    return pipelineLayout;
  }

  void LogicalDevice::destroyPipelineLayout(VkPipelineLayout& pipelineLayout) const
  {
    if (pipelineLayout == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroyPipelineLayout(m_device, pipelineLayout, nullptr);

    pipelineLayout = VK_NULL_HANDLE;
  }

  VkPipeline LogicalDevice::createPipeline(const VkGraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const
  {
    VkPipeline pipeline = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphics pipeline!");
    }

    return pipeline;
  }

  VkPipeline LogicalDevice::createPipeline(const VkComputePipelineCreateInfo& computePipelineCreateInfo) const
  {
    VkPipeline pipeline = VK_NULL_HANDLE;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create compute pipeline!");
    }

    return pipeline;
  }

  void LogicalDevice::destroyPipeline(VkPipeline&pipeline) const
  {
    if (pipeline == VK_NULL_HANDLE)
    {
      return;
    }

    vkDestroyPipeline(m_device, pipeline, nullptr);

    pipeline = VK_NULL_HANDLE;
  }

  VkDeviceAddress LogicalDevice::getBufferDeviceAddress(const VkBuffer& buffer) const
  {
    const VkBufferDeviceAddressInfo bufferDeviceAddressInfo {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buffer
    };

    return vkGetBufferDeviceAddress(m_device, &bufferDeviceAddressInfo);
  }

  void LogicalDevice::createAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& accelerationStructureCreateInfo,
                                                  VkAccelerationStructureKHR* accelerationStructure) const
  {
    m_vkCreateAccelerationStructureKHR(m_device, &accelerationStructureCreateInfo, nullptr, accelerationStructure);
  }

  void LogicalDevice::destroyAccelerationStructureKHR(VkAccelerationStructureKHR& accelerationStructure) const
  {
    if (accelerationStructure == VK_NULL_HANDLE)
    {
      return;
    }

    m_vkDestroyAccelerationStructureKHR(m_device, accelerationStructure, nullptr);

    accelerationStructure = VK_NULL_HANDLE;
  }

  void LogicalDevice::getAccelerationStructureBuildSizes(const VkAccelerationStructureBuildGeometryInfoKHR* accelerationStructureBuildGeometryInfo,
                                                         const uint32_t* maxPrimitiveCounts,
                                                         VkAccelerationStructureBuildSizesInfoKHR* accelerationStructureBuildSizesInfo) const
  {
    m_vkGetAccelerationStructureBuildSizesKHR(
      m_device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      accelerationStructureBuildGeometryInfo,
      maxPrimitiveCounts,
      accelerationStructureBuildSizesInfo
    );
  }

  void LogicalDevice::allocateCommandBuffers(const VkCommandBufferAllocateInfo& commandBufferAllocateInfo,
                                             VkCommandBuffer* commandBuffers) const
  {
    if (vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, commandBuffers) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  }

  void LogicalDevice::freeCommandBuffers(VkCommandPool commandPool,
                                         const uint32_t commandBufferCount,
                                         const VkCommandBuffer* commandBuffers) const
  {
    vkFreeCommandBuffers(m_device, commandPool, commandBufferCount, commandBuffers);
  }

  VkDescriptorPool LogicalDevice::createDescriptorPool(const VkDescriptorPoolCreateInfo& descriptorPoolCreateInfo) const
  {
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    if (vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create descriptor pool!");
    }

    return descriptorPool;
  }

  VkDescriptorSetLayout LogicalDevice::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const
  {
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    if (vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

    return descriptorSetLayout;
  }

  void LogicalDevice::createDevice()
  {
    auto queueFamilyIndices = m_physicalDevice->getQueueFamilies();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set uniqueQueueFamilies = {
      queueFamilyIndices.graphicsFamily.value(),
      queueFamilyIndices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      const VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
      };

      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
      .accelerationStructure = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features vulkan13Features {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &accelerationStructureFeatures,
      .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceVulkan12Features vulkan12Features {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &vulkan13Features,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      .descriptorBindingPartiallyBound = VK_TRUE,
      .runtimeDescriptorArray = VK_TRUE,
      .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceVulkan11Features vulkan11Features {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      .pNext = &vulkan12Features,
      .multiview = VK_TRUE
    };

    VkPhysicalDeviceFeatures2 deviceFeatures2 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &vulkan11Features,
      .features {
        .geometryShader = VK_TRUE,
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
      }
    };

    const VkDeviceCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &deviceFeatures2,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = Instance::validationLayersEnabled() ? static_cast<uint32_t>(validationLayers.size()) : 0,
      .ppEnabledLayerNames = Instance::validationLayersEnabled() ? validationLayers.data() : nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data()
    };

    m_device = m_physicalDevice->createLogicalDevice(createInfo);

    vkGetDeviceQueue(m_device, queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);
    vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);

    loadRayTracingFunctions();
  }

  void LogicalDevice::createSyncObjects()
  {
    m_imageAvailableSemaphores.resize(m_maxFramesInFlight);

    m_renderFinishedSemaphores.resize(m_maxFramesInFlight);
    m_renderFinishedSemaphores2.resize(m_maxFramesInFlight);

    m_computeFinishedSemaphores.resize(m_maxFramesInFlight);

    m_inFlightFences.resize(m_maxFramesInFlight);
    m_inFlightFences2.resize(m_maxFramesInFlight);
    m_mousePickingInFlightFences.resize(m_maxFramesInFlight);
    m_computeInFlightFences.resize(m_maxFramesInFlight);

    constexpr VkSemaphoreCreateInfo semaphoreInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    constexpr VkFenceCreateInfo fenceInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < m_maxFramesInFlight; i++)
    {
      if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores2[i]) != VK_SUCCESS ||
          vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS ||
          vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences2[i]) != VK_SUCCESS ||
          vkCreateFence(m_device, &fenceInfo, nullptr, &m_mousePickingInFlightFences[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create graphics sync objects!");
      }

      if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(m_device, &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create compute sync objects!");
      }
    }
  }

  void LogicalDevice::loadRayTracingFunctions()
  {
    m_vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
      vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));

    m_vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
      vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));

    m_vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
      vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));

    if (!m_vkCreateAccelerationStructureKHR ||
        !m_vkDestroyAccelerationStructureKHR ||
        !m_vkGetAccelerationStructureBuildSizesKHR)
    {
      throw std::runtime_error("Failed to load acceleration structure functions");
    }
  }
} // namespace vke