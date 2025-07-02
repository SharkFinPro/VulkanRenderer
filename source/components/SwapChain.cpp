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
  for (auto imageView : swapChainImageViews)
  {
    logicalDevice->destroyImageView(imageView);
  }

  logicalDevice->destroySwapchainKHR(swapchain);
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

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  int width, height;
  window->getFramebufferSize(&width, &height);

  const VkExtent2D actualExtent {
    .width = std::clamp(static_cast<uint32_t>(width),
                        capabilities.minImageExtent.width,
                        capabilities.maxImageExtent.width),
    .height = std::clamp(static_cast<uint32_t>(height),
                         capabilities.minImageExtent.height,
                         capabilities.maxImageExtent.height)
  };

  return actualExtent;
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

  const auto indices = physicalDevice->getQueueFamilies();
  const uint32_t queueFamilyIndices[] = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  const VkSwapchainCreateInfoKHR createInfo {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = window->getSurface(),
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = indices.graphicsFamily != indices.presentFamily ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = static_cast<uint32_t>(indices.graphicsFamily != indices.presentFamily ? 2 : 1),
    .pQueueFamilyIndices = indices.graphicsFamily != indices.presentFamily ? queueFamilyIndices : nullptr,
    .preTransform = swapChainSupport.capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  swapchain = logicalDevice->createSwapchain(createInfo);

  logicalDevice->getSwapchainImagesKHR(swapchain, &imageCount, nullptr);
  swapChainImageViews.resize(imageCount);
  logicalDevice->getSwapchainImagesKHR(swapchain, &imageCount, swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void SwapChain::createImageViews()
{
  swapChainImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++)
  {
    swapChainImageViews[i] = Images::createImageView(logicalDevice, swapChainImages[i], swapChainImageFormat,
                                                     VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);
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
