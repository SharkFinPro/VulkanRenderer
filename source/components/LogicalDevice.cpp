#include "LogicalDevice.h"

#include <array>

#include "PhysicalDevice.h"
#include "Instance.h"
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

VkDevice LogicalDevice::getDevice() const
{
  return device;
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