#include "LogicalDevice.h"

#include <array>

#include "PhysicalDevice.h"
#include "../core/instance/Instance.h"
#include <set>
#include <stdexcept>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

LogicalDevice::LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
{
  createDevice(physicalDevice);

  createSyncObjects();
}

LogicalDevice::~LogicalDevice()
{
  for (size_t i = 0; i < maxFramesInFlight; i++)
  {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphores2[i], nullptr);

    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);

    vkDestroySemaphore(device, computeFinishedSemaphores[i], nullptr);

    vkDestroyFence(device, inFlightFences[i], nullptr);
    vkDestroyFence(device, inFlightFences2[i], nullptr);
    vkDestroyFence(device, mousePickingInFlightFences[i], nullptr);
    vkDestroyFence(device, computeInFlightFences[i], nullptr);
  }

  vkDestroyDevice(device, nullptr);
}

void LogicalDevice::waitIdle() const
{
  vkDeviceWaitIdle(device);
}

VkQueue LogicalDevice::getGraphicsQueue() const
{
  return graphicsQueue;
}

VkQueue LogicalDevice::getPresentQueue() const
{
  return presentQueue;
}

VkQueue LogicalDevice::getComputeQueue() const
{
  return computeQueue;
}

void LogicalDevice::submitMousePickingGraphicsQueue(const uint32_t currentFrame, const VkCommandBuffer *commandBuffer) const
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

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, mousePickingInFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}

void LogicalDevice::submitOffscreenGraphicsQueue(const uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const
{
  const std::array<VkSemaphore, 1> waitSemaphores = {
    computeFinishedSemaphores[currentFrame]
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
    .pSignalSemaphores = &renderFinishedSemaphores2[currentFrame]
  };

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences2[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}

void LogicalDevice::submitGraphicsQueue(const uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const
{
  const std::array<VkSemaphore, 1> waitSemaphores = {
    imageAvailableSemaphores[currentFrame]
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
    .pSignalSemaphores = &renderFinishedSemaphores[currentFrame]
  };

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}

void LogicalDevice::submitComputeQueue(const uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const
{
  const VkSubmitInfo submitInfo {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &computeFinishedSemaphores[currentFrame]
  };

  if (vkQueueSubmit(computeQueue, 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit compute command buffer!");
  }
}

void LogicalDevice::waitForGraphicsFences(const uint32_t currentFrame) const
{
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkWaitForFences(device, 1, &inFlightFences2[currentFrame], VK_TRUE, UINT64_MAX);
  vkWaitForFences(device, 1, &mousePickingInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}
void LogicalDevice::waitForComputeFences(const uint32_t currentFrame) const
{
  vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

void LogicalDevice::waitForMousePickingFences(uint32_t currentFrame) const
{
  vkWaitForFences(device, 1, &mousePickingInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

void LogicalDevice::resetGraphicsFences(const uint32_t currentFrame) const
{
  vkResetFences(device, 1, &inFlightFences[currentFrame]);
  vkResetFences(device, 1, &inFlightFences2[currentFrame]);
  vkResetFences(device, 1, &mousePickingInFlightFences[currentFrame]);
}

void LogicalDevice::resetComputeFences(const uint32_t currentFrame) const
{
  vkResetFences(device, 1, &computeInFlightFences[currentFrame]);
}

VkResult LogicalDevice::queuePresent(const uint32_t currentFrame, const VkSwapchainKHR& swapchain, const uint32_t* imageIndex) const
{
  const std::array<VkSemaphore, 2> waitSemaphores = {
    renderFinishedSemaphores[currentFrame],
    renderFinishedSemaphores2[currentFrame]
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

  return vkQueuePresentKHR(presentQueue, &presentInfo);
}

VkResult LogicalDevice::acquireNextImage(const uint32_t currentFrame, const VkSwapchainKHR& swapchain, uint32_t* imageIndex) const
{
  return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                               imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
}

uint32_t LogicalDevice::getMaxFramesInFlight() const
{
  return maxFramesInFlight;
}

VkCommandPool LogicalDevice::createCommandPool(const VkCommandPoolCreateInfo& commandPoolCreateInfo) const
{
  VkCommandPool commandPool = VK_NULL_HANDLE;

  if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }

  return commandPool;
}

void LogicalDevice::destroyCommandPool(VkCommandPool& commandPool) const
{
  vkDestroyCommandPool(device, commandPool, nullptr);

  commandPool = VK_NULL_HANDLE;
}

void LogicalDevice::destroyDescriptorPool(VkDescriptorPool& descriptorPool) const
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  descriptorPool = VK_NULL_HANDLE;
}

void LogicalDevice::destroyDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout) const
{
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

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

void LogicalDevice::mapMemory(const VkDeviceMemory& memory, const VkDeviceSize offset, const VkDeviceSize size,
                              const VkMemoryMapFlags flags, void** data) const
{
  vkMapMemory(device, memory, offset, size, flags, data);
}

void LogicalDevice::unmapMemory(VkDeviceMemory& memory) const
{
  if (memory == VK_NULL_HANDLE)
  {
    return;
  }

  vkUnmapMemory(device, memory);

  memory = VK_NULL_HANDLE;
}

void LogicalDevice::allocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo,
                                           VkDescriptorSet* descriptorSets) const
{
  if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}

void LogicalDevice::updateDescriptorSets(const uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet* descriptorWrites) const
{
  vkUpdateDescriptorSets(device, descriptorWriteCount, descriptorWrites, 0, nullptr);
}

VkBuffer LogicalDevice::createBuffer(const VkBufferCreateInfo& bufferCreateInfo) const
{
  VkBuffer buffer = VK_NULL_HANDLE;

  if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
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

  vkDestroyBuffer(device, buffer, nullptr);

  buffer = VK_NULL_HANDLE;
}

VkMemoryRequirements LogicalDevice::getBufferMemoryRequirements(const VkBuffer& buffer) const
{
  VkMemoryRequirements memoryRequirements{};

  vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

  return memoryRequirements;
}

void LogicalDevice::allocateMemory(const VkMemoryAllocateInfo& memoryAllocateInfo, VkDeviceMemory& deviceMemory) const
{
  if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS)
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

  vkFreeMemory(device, memory, nullptr);

  memory = VK_NULL_HANDLE;
}

void LogicalDevice::bindBufferMemory(const VkBuffer& buffer, const VkDeviceMemory& deviceMemory,
                                     const VkDeviceSize memoryOffset) const
{
  vkBindBufferMemory(device, buffer, deviceMemory, memoryOffset);
}

VkSampler LogicalDevice::createSampler(const VkSamplerCreateInfo &samplerCreateInfo) const
{
  VkSampler sampler = VK_NULL_HANDLE;

  if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create sampler!");
  }

  return sampler;
}

void LogicalDevice::destroySampler(VkSampler& sampler) const
{
  vkDestroySampler(device, sampler, nullptr);

  sampler = VK_NULL_HANDLE;
}

VkImageView LogicalDevice::createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const
{
  VkImageView imageView = VK_NULL_HANDLE;

  if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image view!");
  }

  return imageView;
}

void LogicalDevice::destroyImageView(VkImageView& imageView) const
{
  vkDestroyImageView(device, imageView, nullptr);

  imageView = VK_NULL_HANDLE;
}

VkImage LogicalDevice::createImage(const VkImageCreateInfo& imageCreateInfo) const
{
  VkImage image = VK_NULL_HANDLE;

  if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image!");
  }

  return image;
}

void LogicalDevice::destroyImage(VkImage& image) const
{
  vkDestroyImage(device, image, nullptr);

  image = VK_NULL_HANDLE;
}

VkMemoryRequirements LogicalDevice::getImageMemoryRequirements(const VkImage& image) const
{
  VkMemoryRequirements memoryRequirements{};

  vkGetImageMemoryRequirements(device, image, &memoryRequirements);

  return memoryRequirements;
}

void LogicalDevice::bindImageMemory(const VkImage& image, const VkDeviceMemory& deviceMemory,
                                    const VkDeviceSize memoryOffset) const
{
  vkBindImageMemory(device, image, deviceMemory, memoryOffset);
}

VkRenderPass LogicalDevice::createRenderPass(const VkRenderPassCreateInfo &renderPassCreateInfo) const
{
  VkRenderPass renderPass = VK_NULL_HANDLE;

  if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }

  return renderPass;
}

void LogicalDevice::destroyRenderPass(VkRenderPass& renderPass) const
{
  vkDestroyRenderPass(device, renderPass, nullptr);

  renderPass = VK_NULL_HANDLE;
}

VkShaderModule LogicalDevice::createShaderModule(const VkShaderModuleCreateInfo &shaderModuleCreateInfo) const
{
  VkShaderModule shaderModule = VK_NULL_HANDLE;

  if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void LogicalDevice::destroyShaderModule(VkShaderModule& shaderModule) const
{
  vkDestroyShaderModule(device, shaderModule, nullptr);

  shaderModule = VK_NULL_HANDLE;
}

VkSwapchainKHR LogicalDevice::createSwapchain(const VkSwapchainCreateInfoKHR& swapchainCreateInfo) const
{
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create swapchain!");
  }

  return swapchain;
}

void LogicalDevice::getSwapchainImagesKHR(const VkSwapchainKHR& swapchain, uint32_t* swapchainImageCount,
                                          VkImage* swapchainImages) const
{
  vkGetSwapchainImagesKHR(device, swapchain, swapchainImageCount, swapchainImages);
}

void LogicalDevice::destroySwapchainKHR(VkSwapchainKHR& swapchain) const
{
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  swapchain = VK_NULL_HANDLE;
}

VkFramebuffer LogicalDevice::createFramebuffer(const VkFramebufferCreateInfo& framebufferCreateInfo) const
{
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create framebuffer!");
  }

  return framebuffer;
}

void LogicalDevice::destroyFramebuffer(VkFramebuffer& framebuffer) const
{
  vkDestroyFramebuffer(device, framebuffer, nullptr);

  framebuffer = VK_NULL_HANDLE;
}

VkPipelineLayout LogicalDevice::createPipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const
{
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

  if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

  pipelineLayout = VK_NULL_HANDLE;
}

VkPipeline LogicalDevice::createPipeline(const VkGraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const
{
  VkPipeline pipeline = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  return pipeline;
}

VkPipeline LogicalDevice::createPipeline(const VkComputePipelineCreateInfo& computePipelineCreateInfo) const
{
  VkPipeline pipeline = VK_NULL_HANDLE;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
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

  vkDestroyPipeline(device, pipeline, nullptr);

  pipeline = VK_NULL_HANDLE;
}

void LogicalDevice::allocateCommandBuffers(const VkCommandBufferAllocateInfo& commandBufferAllocateInfo,
                                           VkCommandBuffer* commandBuffers) const
{
  if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void LogicalDevice::freeCommandBuffers(VkCommandPool commandPool, const uint32_t commandBufferCount,
                                       const VkCommandBuffer* commandBuffers) const
{
  vkFreeCommandBuffers(device, commandPool, commandBufferCount, commandBuffers);
}

VkDescriptorPool LogicalDevice::createDescriptorPool(const VkDescriptorPoolCreateInfo& descriptorPoolCreateInfo) const
{
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  return descriptorPool;
}

VkDescriptorSetLayout LogicalDevice::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const
{
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  return descriptorSetLayout;
}

void LogicalDevice::createDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
{
  auto queueFamilyIndices = physicalDevice->getQueueFamilies();

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

  constexpr VkPhysicalDeviceFeatures deviceFeatures {
    .geometryShader = VK_TRUE,
    .samplerAnisotropy = VK_TRUE
  };

  const VkDeviceCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
    .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
    .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures = &deviceFeatures
  };

  device = physicalDevice->createLogicalDevice(createInfo);

  vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
  vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

void LogicalDevice::createSyncObjects()
{
  imageAvailableSemaphores.resize(maxFramesInFlight);

  renderFinishedSemaphores.resize(maxFramesInFlight);
  renderFinishedSemaphores2.resize(maxFramesInFlight);

  computeFinishedSemaphores.resize(maxFramesInFlight);

  inFlightFences.resize(maxFramesInFlight);
  inFlightFences2.resize(maxFramesInFlight);
  mousePickingInFlightFences.resize(maxFramesInFlight);
  computeInFlightFences.resize(maxFramesInFlight);

  constexpr VkSemaphoreCreateInfo semaphoreInfo {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };

  constexpr VkFenceCreateInfo fenceInfo {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };

  for (size_t i = 0; i < maxFramesInFlight; i++)
  {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores2[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences2[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &mousePickingInFlightFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphics sync objects!");
    }

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create compute sync objects!");
    }
  }
}