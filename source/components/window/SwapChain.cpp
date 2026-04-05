#include "SwapChain.h"
#include "Surface.h"
#include "Window.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../../utilities/Images.h"
#include <limits>
#include <algorithm>

namespace vke {

  SwapChain::SwapChain(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<Window>& window,
                       const std::shared_ptr<Surface>& surface)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createSwapChain(window, surface);
    createImageViews();
  }

  vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
  {
    for (const auto& availableFormat : availableFormats)
    {
      if (availableFormat.format == vk::Format::eR8G8B8A8Unorm &&
          availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      {
        return availableFormat;
      }
    }

    return availableFormats[0];
  }

  vk::PresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
  {
    for (const auto& availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox)
      {
        return availablePresentMode;
      }
    }

    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                           const std::shared_ptr<Window>& window)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }

    int width, height;
    window->getFramebufferSize(&width, &height);

    const vk::Extent2D actualExtent {
      .width = std::clamp(static_cast<uint32_t>(width),
                          capabilities.minImageExtent.width,
                          capabilities.maxImageExtent.width),
      .height = std::clamp(static_cast<uint32_t>(height),
                           capabilities.minImageExtent.height,
                           capabilities.maxImageExtent.height)
    };

    return actualExtent;
  }

  uint32_t SwapChain::chooseSwapImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
  {
    const uint32_t imageCount = capabilities.minImageCount + 1;

    const bool imageCountExceeded = capabilities.maxImageCount > 0 &&
                                    imageCount > capabilities.maxImageCount;

    return imageCountExceeded ? capabilities.maxImageCount : imageCount;
  }

  void SwapChain::createSwapChain(const std::shared_ptr<Window>& window,
                                  const std::shared_ptr<Surface>& surface)
  {
    const SwapChainSupportDetails swapChainSupport = m_logicalDevice->getPhysicalDevice()->getSwapChainSupport();

    const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
    const uint32_t imageCount = chooseSwapImageCount(swapChainSupport.capabilities);

    const auto indices = m_logicalDevice->getPhysicalDevice()->getQueueFamilies();
    const uint32_t queueFamilyIndices[] = {
      indices.graphicsFamily.value(),
      indices.presentFamily.value()
    };

    const bool concurrentSharing = indices.graphicsFamily != indices.presentFamily;

    const vk::SwapchainCreateInfoKHR createInfo {
      .surface = surface->getSurface(),
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = concurrentSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = concurrentSharing ? 2u : 1u,
      .pQueueFamilyIndices = concurrentSharing ? queueFamilyIndices : nullptr,
      .preTransform = swapChainSupport.capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = vk::True,
      .oldSwapchain = nullptr
    };

    m_swapchain = m_logicalDevice->createSwapchain(createInfo);

    m_swapChainImages = m_swapchain.getImages();
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
  }

  void SwapChain::createImageViews()
  {
    m_swapChainImageViews.reserve(m_swapChainImages.size());

    for (const auto& image : m_swapChainImages)
    {
      m_swapChainImageViews.push_back(
        Images::createImageView(
          m_logicalDevice,
          image,
          m_swapChainImageFormat,
          vk::ImageAspectFlagBits::eColor,
          1,
          vk::ImageViewType::e2D,
          1
        )
      );
    }
  }

  vk::Format SwapChain::getImageFormat() const
  {
    return m_swapChainImageFormat;
  }

  vk::Extent2D SwapChain::getExtent() const
  {
    return m_swapChainExtent;
  }

  vk::raii::SwapchainKHR& SwapChain::getSwapChain()
  {
    return m_swapchain;
  }

  const std::vector<vk::raii::ImageView>& SwapChain::getImageViews() const
  {
    return m_swapChainImageViews;
  }

  const std::vector<vk::Image>& SwapChain::getImages() const
  {
    return m_swapChainImages;
  }

} // namespace vke