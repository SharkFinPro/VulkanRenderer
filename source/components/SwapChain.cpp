#include "SwapChain.h"
#include "../utilities/Images.h"

#include <stdexcept>
#include <limits>
#include <algorithm>

SwapChain::SwapChain(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                     const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const std::shared_ptr<Window>& window)
  : physicalDevice(physicalDevice), logicalDevice(logicalDevice), window(window)
{
  createSwapChain();
  createImageViews();
}

SwapChain::~SwapChain()
{
  for (const auto imageView : swapChainImageViews)
  {
    vkDestroyImageView(logicalDevice->getDevice(), imageView, nullptr);
  }

  vkDestroySwapchainKHR(logicalDevice->getDevice(), swapchain, nullptr);
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  for (const auto& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    window->getFramebufferSize(&width, &height);

    VkExtent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

uint32_t SwapChain::chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
  const uint32_t imageCount = capabilities.minImageCount + 1;

  const bool imageCountExceeded = capabilities.maxImageCount && imageCount > capabilities.maxImageCount;

  return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}

void SwapChain::createSwapChain()
{
  SwapChainSupportDetails swapChainSupport = physicalDevice->getSwapChainSupport();

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
  uint32_t imageCount = chooseSwapImageCount(swapChainSupport.capabilities);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = window->getSurface();

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto indices = physicalDevice->getQueueFamilies();
  const uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(logicalDevice->getDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(logicalDevice->getDevice(), swapchain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice->getDevice(), swapchain, &imageCount, swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void SwapChain::createImageViews()
{
  swapChainImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++)
  {
    swapChainImageViews[i] = Images::createImageView(logicalDevice->getDevice(), swapChainImages[i], swapChainImageFormat,
                                                     VK_IMAGE_ASPECT_COLOR_BIT, 1);
  }
}

VkFormat& SwapChain::getImageFormat()
{
  return swapChainImageFormat;
}

VkExtent2D& SwapChain::getExtent()
{
  return swapChainExtent;
}

VkSwapchainKHR& SwapChain::getSwapChain()
{
  return swapchain;
}

std::vector<VkImageView>& SwapChain::getImageViews()
{
  return swapChainImageViews;
}
