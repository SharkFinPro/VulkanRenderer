#include "Renderer.h"
#include "ImageResource.h"
#include "RenderTarget.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../lighting/lights/Light.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../window/SwapChain.h"

namespace vke {

  Renderer::Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                     const std::shared_ptr<SwapChain>& swapChain,
                     const vk::CommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    createSampler();

    createSwapchainRenderTarget(swapChain);

    createMousePickingRenderTarget(swapChain->getExtent());
  }

  vk::DescriptorSet Renderer::getOffscreenImageDescriptorSet(const uint32_t imageIndex) const
  {
    if (!m_offscreenRenderTarget)
    {
      return nullptr;
    }

    return m_offscreenRenderTarget->getResolveImageResource(imageIndex).getDescriptorSet();
  }

  std::shared_ptr<RenderTarget> Renderer::getMousePickingRenderTarget() const
  {
    return m_mousePickingRenderTarget;
  }

  std::shared_ptr<RenderTarget> Renderer::getRayTracingRenderTarget() const
  {
    return m_rayTracingRenderTarget;
  }

  void Renderer::resetSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain)
  {
    m_swapchainRenderTarget.reset();

    createSwapchainRenderTarget(swapChain);
  }

  void Renderer::resetOffscreenRenderTarget(const vk::Extent2D offscreenViewportExtent)
  {
    m_offscreenRenderTarget.reset();

    createOffscreenRenderTarget(offscreenViewportExtent);

    resetRayTracingRenderTarget(offscreenViewportExtent);
  }

  void Renderer::resetMousePickingRenderTarget(const vk::Extent2D mousePickingExtent)
  {
    m_mousePickingRenderTarget.reset();

    createMousePickingRenderTarget(mousePickingExtent);
  }

  void Renderer::beginSwapchainRendering(const uint32_t imageIndex,
                                         const std::shared_ptr<CommandBuffer>& commandBuffer,
                                         const std::shared_ptr<SwapChain>& swapChain) const
  {
    transitionSwapchainImagePreRender(commandBuffer, swapChain->getImages()[imageIndex]);

    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_swapchainRenderTarget->getColorImageResource(imageIndex).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eAverage,
      .resolveImageView = swapChain->getImageViews()[imageIndex],
      .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_swapchainRenderTarget->getDepthImageResource(imageIndex).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_swapchainRenderTarget->getExtent(),
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void Renderer::beginOffscreenRendering(const uint32_t currentFrame,
                                         const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_offscreenRenderTarget->getColorImageResource(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eAverage,
      .resolveImageView = m_offscreenRenderTarget->getResolveImageResource(currentFrame).getImageView(),
      .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_offscreenRenderTarget->getDepthImageResource(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_offscreenRenderTarget->getExtent(),
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void Renderer::beginShadowRendering(const vk::Extent2D extent,
                                      const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const std::shared_ptr<Light>& light)
  {
    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = light->getShadowMapView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearDepth
    };

    constexpr uint32_t kCubemapFacesMask = 0x3Fu;
    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .viewMask = light->getLightType() == LightType::pointLight ? kCubemapFacesMask : 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void Renderer::beginMousePickingRendering(const uint32_t currentFrame,
                                            const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_mousePickingRenderTarget->getColorImageResource(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eNone,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_mousePickingRenderTarget->getDepthImageResource(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_mousePickingRenderTarget->getExtent(),
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void Renderer::endSwapchainRendering(const uint32_t imageIndex,
                                       const std::shared_ptr<CommandBuffer>& commandBuffer,
                                       const std::shared_ptr<SwapChain>& swapChain)
  {
    commandBuffer->endRendering();

    transitionSwapchainImagePostRender(commandBuffer, swapChain->getImages()[imageIndex]);
  }

  void Renderer::beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          const uint32_t currentFrame) const
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eShaderWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eGeneral,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = m_rayTracingRenderTarget->getRayTracingImageResource(currentFrame).getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    commandBuffer->pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::PipelineStageFlagBits::eRayTracingShaderKHR,
      {},
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void Renderer::endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                        const uint32_t currentFrame) const
  {
    transitionRayTracingImagePreCopy(commandBuffer, currentFrame);

    copyRayTracingImageToOffscreenImage(commandBuffer, currentFrame);

    transitionRayTracingImagePostCopy(commandBuffer, currentFrame);
  }

  void Renderer::resetRayTracingRenderTarget(const vk::Extent2D extent)
  {
    m_rayTracingRenderTarget.reset();

    createRayTracingRenderTarget(extent);
  }

  void Renderer::createSampler()
  {
    constexpr vk::SamplerCreateInfo samplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eClampToEdge,
      .addressModeV = vk::SamplerAddressMode::eClampToEdge,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::False,
      .maxAnisotropy = 1.0f,
      .compareEnable = vk::False,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = vk::False
    };

    m_sampler = m_logicalDevice->createSampler(samplerInfo);
  }

  void Renderer::createSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = swapChain->getExtent(),
      .commandPool = m_commandPool,
      .colorFormat = swapChain->getImageFormat(),
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples()
    };

    m_swapchainRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, static_cast<uint32_t>(swapChain->getImages().size()));
  }

  void Renderer::createOffscreenRenderTarget(const vk::Extent2D extent)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = vk::Format::eR8G8B8A8Unorm,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .resolveFormat = vk::Format::eR8G8B8A8Unorm,
      .numSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .sampler = m_sampler
    };

    m_offscreenRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, m_logicalDevice->getMaxFramesInFlight());
  }

  void Renderer::createMousePickingRenderTarget(const vk::Extent2D extent)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = vk::Format::eR8G8B8A8Uint,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = vk::SampleCountFlagBits::e1
    };

    m_mousePickingRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, m_logicalDevice->getMaxFramesInFlight());
  }

  void Renderer::createRayTracingRenderTarget(const vk::Extent2D extent)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }
    
    ImageResourceConfig imageResourceConfig {
      .imageResourceType = ImageResourceType::RayTracingOutput,
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .resolveFormat = vk::Format::eR8G8B8A8Unorm,
      .numSamples = vk::SampleCountFlagBits::e1
    };

    m_rayTracingRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, m_logicalDevice->getMaxFramesInFlight());
  }

  void Renderer::transitionSwapchainImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   const vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
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

    commandBuffer->pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      {},
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void Renderer::transitionSwapchainImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                    const vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eNone,
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

    commandBuffer->pipelineBarrier(
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eBottomOfPipe,
      {},
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void Renderer::transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  const uint32_t currentFrame) const
  {
    const auto rtImage = m_rayTracingRenderTarget->getRayTracingImageResource(currentFrame).getImage();
    const auto offscreenImage = m_offscreenRenderTarget->getResolveImageResource(currentFrame).getImage();

    // Transition RT image: GENERAL -> TRANSFER_SRC_OPTIMAL
    // Transition offscreen resolve image: UNDEFINED -> TRANSFER_DST_OPTIMAL
    const std::vector preTransferBarriers {
      vk::ImageMemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eTransferRead,
        .oldLayout = vk::ImageLayout::eGeneral,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = rtImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      },
      vk::ImageMemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = offscreenImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      }
    };

    commandBuffer->pipelineBarrier(
      vk::PipelineStageFlagBits::eRayTracingShaderKHR,
      vk::PipelineStageFlagBits::eTransfer,
      {},
      {},
      {},
      preTransferBarriers
    );
  }

  void Renderer::transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   const uint32_t currentFrame) const
  {
    const auto rtImage = m_rayTracingRenderTarget->getRayTracingImageResource(currentFrame).getImage();
    const auto offscreenImage = m_offscreenRenderTarget->getResolveImageResource(currentFrame).getImage();

    // Transition offscreen resolve: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    // Transition RT image back: TRANSFER_SRC_OPTIMAL -> GENERAL
    const std::vector postTransferBarriers {
      vk::ImageMemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = offscreenImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      },
      vk::ImageMemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferRead,
        .dstAccessMask = vk::AccessFlagBits::eNone,
        .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = rtImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      }
    };

    commandBuffer->pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eFragmentShader,
      {},
      {},
      {},
      postTransferBarriers
    );
  }

  void Renderer::copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                     const uint32_t currentFrame) const
  {
    const auto rtImage = m_rayTracingRenderTarget->getRayTracingImageResource(currentFrame).getImage();
    const auto offscreenImage = m_offscreenRenderTarget->getResolveImageResource(currentFrame).getImage();

    const auto extent = m_offscreenRenderTarget->getExtent();

    const vk::ImageCopy imageCopy {
      .srcSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
      .srcOffset = { 0, 0, 0 },
      .dstSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
      .dstOffset = { 0, 0, 0 },
      .extent = { extent.width, extent.height, 1 }
    };

    commandBuffer->copyImage(
      rtImage,
      vk::ImageLayout::eTransferSrcOptimal,
      offscreenImage,
      vk::ImageLayout::eTransferDstOptimal,
      { imageCopy }
    );
  }

} // namespace vke