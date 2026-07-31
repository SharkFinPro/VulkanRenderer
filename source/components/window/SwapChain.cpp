#include "SwapChain.h"
#include "Surface.h"
#include "Window.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../commandBuffer/SingleUseCommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../renderingManager/ImageResource.h"
#include "../../utilities/Images.h"
#include <limits>
#include <algorithm>

namespace vke {

  SwapChain::SwapChain(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const std::shared_ptr<Window>& window,
                       const std::shared_ptr<Surface>& surface,
                       const vk::CommandPool commandPool,
                       const vk::SwapchainKHR oldSwapchain)
  {
    createSwapChain(logicalDevice, window, surface, oldSwapchain);

    createImageViews(logicalDevice);

    createImageResources(logicalDevice, commandPool);
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

  void SwapChain::createSwapChain(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                  const std::shared_ptr<Window>& window,
                                  const std::shared_ptr<Surface>& surface,
                                  const vk::SwapchainKHR oldSwapchain)
  {
    const SwapChainSupportDetails swapChainSupport = logicalDevice->getPhysicalDevice()->getSwapChainSupport();

    const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
    const uint32_t imageCount = chooseSwapImageCount(swapChainSupport.capabilities);

    const auto indices = logicalDevice->getPhysicalDevice()->getQueueFamilies();
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
      .oldSwapchain = oldSwapchain
    };

    m_swapchain = logicalDevice->createSwapchain(createInfo);

    m_swapChainImages = m_swapchain.getImages();
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
  }

  void SwapChain::createImageViews(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    m_swapChainImageViews.reserve(m_swapChainImages.size());

    for (const auto& image : m_swapChainImages)
    {
      m_swapChainImageViews.push_back(
        Images::createImageView(
          logicalDevice,
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

  void SwapChain::createImageResources(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                       const vk::CommandPool commandPool)
  {
    // One command buffer for every image's initial transition, rather than a submit and queue
    // drain per image.
    const auto batchCommandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    batchCommandBuffer.record([this, &logicalDevice, commandPool, &batchCommandBuffer] {
      const ImageResourceConfig imageResourceConfig {
        .logicalDevice = logicalDevice,
        .extent = m_swapChainExtent,
        .commandPool = commandPool,
        .colorFormat = m_swapChainImageFormat,
        .depthFormat = logicalDevice->getPhysicalDevice()->findDepthFormat(),
        .numSamples = logicalDevice->getPhysicalDevice()->getMsaaSamples(),
        .batchCommandBuffer = &batchCommandBuffer
      };

      auto colorImageResourceConfig = imageResourceConfig;
      colorImageResourceConfig.imageResourceType = ImageResourceType::Color;

      auto depthImageResourceConfig = imageResourceConfig;
      depthImageResourceConfig.imageResourceType = ImageResourceType::Depth;

      const auto numImages = m_swapChainImages.size();
      m_colorImageResources.reserve(numImages);
      m_depthImageResources.reserve(numImages);

      for (size_t i = 0; i < numImages; ++i)
      {
        m_colorImageResources.emplace_back(colorImageResourceConfig);
        m_depthImageResources.emplace_back(depthImageResourceConfig);
      }
    });
  }

  void SwapChain::transitionImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           const vk::Image image)
  {
    // srcStageMask chains after the image-acquire semaphore wait (eColorAttachmentOutput).
    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eNone,
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void SwapChain::transitionImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                            const vk::Image image)
  {
    // No destination scope: visibility to the presentation engine is handled by the
    // render-finished semaphore signal.
    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eNone,
      .dstAccessMask = vk::AccessFlagBits2::eNone,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::ePresentSrcKHR,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  vk::Format SwapChain::getImageFormat() const
  {
    return m_swapChainImageFormat;
  }

  vk::Extent2D SwapChain::getExtent() const
  {
    return m_swapChainExtent;
  }

  vk::SwapchainKHR SwapChain::getSwapChain() const
  {
    return *m_swapchain;
  }

  const std::vector<vk::raii::ImageView>& SwapChain::getImageViews() const
  {
    return m_swapChainImageViews;
  }

  const std::vector<vk::Image>& SwapChain::getImages() const
  {
    return m_swapChainImages;
  }

  void SwapChain::beginRendering(const uint32_t imageIndex,
                                 const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    static constexpr vk::ClearValue s_clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    transitionImagePreRender(commandBuffer, m_swapChainImages.at(imageIndex));

    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_colorImageResources.at(imageIndex).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eAverage,
      .resolveImageView = m_swapChainImageViews.at(imageIndex),
      .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_depthImageResources.at(imageIndex).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_swapChainExtent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void SwapChain::endRendering(const uint32_t imageIndex,
                               const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    commandBuffer->endRendering();

    transitionImagePostRender(commandBuffer, m_swapChainImages.at(imageIndex));
  }

} // namespace vke