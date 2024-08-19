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

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

LogicalDevice::LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
{
  createDevice(physicalDevice);

  createSyncObjects();
}

LogicalDevice::~LogicalDevice()
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, computeFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
    vkDestroyFence(device, computeInFlightFences[i], nullptr);
  }

  vkDestroyDevice(device, nullptr);
}

VkDevice& LogicalDevice::getDevice()
{
  return device;
}

void LogicalDevice::waitIdle() const
{
  vkDeviceWaitIdle(device);
}

VkQueue& LogicalDevice::getGraphicsQueue()
{
  return graphicsQueue;
}

VkQueue& LogicalDevice::getPresentQueue()
{
  return presentQueue;
}

VkQueue& LogicalDevice::getComputeQueue()
{
  return computeQueue;
}

void LogicalDevice::createDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice)
{
  auto queueFamilyIndices = physicalDevice->getQueueFamilies();

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(),
                                            queueFamilyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamily,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority
    };

    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures {
    .samplerAnisotropy = VK_TRUE
  };

  VkDeviceCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
    .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
    .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures = &deviceFeatures
  };

  if (vkCreateDevice(physicalDevice->getPhysicalDevice(), &createInfo, nullptr, &device) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
  vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

void LogicalDevice::createSyncObjects()
{
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
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

void LogicalDevice::submitGraphicsQueue(const uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const
{
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  const std::array<VkSemaphore, 2> waitSemaphores = {
    computeFinishedSemaphores[currentFrame],
    imageAvailableSemaphores[currentFrame]
  };
  constexpr VkPipelineStageFlags waitStages[] = {
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}
void LogicalDevice::submitComputeQueue(const uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const
{
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

  if (vkQueueSubmit(computeQueue, 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit compute command buffer!");
  }
}

void LogicalDevice::waitForGraphicsFences(const uint32_t currentFrame) const
{
  vkWaitForFences(device, 1, &inFlightFences[currentFrame],
    VK_TRUE, UINT64_MAX);
}
void LogicalDevice::waitForComputeFences(const uint32_t currentFrame) const
{
  vkWaitForFences(device, 1, &computeInFlightFences[currentFrame],
    VK_TRUE, UINT64_MAX);
}

void LogicalDevice::resetGraphicsFences(const uint32_t currentFrame) const
{
  vkResetFences(device, 1, &inFlightFences[currentFrame]);
}
void LogicalDevice::resetComputeFences(const uint32_t currentFrame) const
{
  vkResetFences(device, 1, &computeInFlightFences[currentFrame]);
}

VkResult LogicalDevice::queuePresent(const uint32_t currentFrame, const VkSwapchainKHR& swapchain, const uint32_t* imageIndex) const
{
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = imageIndex;

  presentInfo.pResults = nullptr;

  return vkQueuePresentKHR(presentQueue, &presentInfo);
}

VkResult LogicalDevice::acquireNextImage(const uint32_t currentFrame, const VkSwapchainKHR& swapchain, uint32_t* imageIndex) const
{
  return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                               imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
}
