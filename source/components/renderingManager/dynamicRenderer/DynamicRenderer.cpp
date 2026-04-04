#include "DynamicRenderer.h"
#include "../ImageResource.h"
#include "../RenderTarget.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/lights/Light.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../window/SwapChain.h"

namespace vke {

  DynamicRenderer::DynamicRenderer(std::shared_ptr<LogicalDevice> logicalDevice,
                                   const std::shared_ptr<SwapChain>& swapChain,
                                   vk::raii::CommandPool commandPool)
    : Renderer(std::move(logicalDevice), swapChain, std::move(commandPool))
  {}

  void DynamicRenderer::beginSwapchainRendering(const uint32_t imageIndex,
                                                const vk::Extent2D extent,
                                                const std::shared_ptr<CommandBuffer> commandBuffer,
                                                const std::shared_ptr<SwapChain> swapChain)
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
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginOffscreenRendering(const uint32_t currentFrame,
                                                const vk::Extent2D extent,
                                                const std::shared_ptr<CommandBuffer> commandBuffer)
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
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginShadowRendering(uint32_t currentFrame,
                                             const vk::Extent2D extent,
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

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .viewMask = light->getLightType() == LightType::pointLight ? 0x3Fu : 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginMousePickingRendering(const uint32_t currentFrame,
                                                   const vk::Extent2D extent,
                                                   const std::shared_ptr<CommandBuffer>& commandBuffer)
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
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::endSwapchainRendering(const uint32_t imageIndex,
                                              const std::shared_ptr<CommandBuffer> commandBuffer,
                                              const std::shared_ptr<SwapChain> swapChain)
  {
    commandBuffer->endRendering();

    transitionSwapchainImagePostRender(commandBuffer, swapChain->getImages()[imageIndex]);
  }

  void DynamicRenderer::endOffscreenRendering(const std::shared_ptr<CommandBuffer> commandBuffer)
  {
    commandBuffer->endRendering();
  }

  void DynamicRenderer::endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    commandBuffer->endRendering();
  }

  void DynamicRenderer::endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    commandBuffer->endRendering();
  }

  bool DynamicRenderer::supportsRayTracing() const
  {
    return m_logicalDevice->getPhysicalDevice()->supportsRayTracing();
  }

  void DynamicRenderer::beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                 const uint32_t currentFrame)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eShaderWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eGeneral,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = getRayTracingImageResource(currentFrame)->getImage(),
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

  void DynamicRenderer::endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                               const uint32_t currentFrame)
  {
    transitionRayTracingImagePreCopy(commandBuffer, currentFrame);

    copyRayTracingImageToOffscreenImage(commandBuffer, currentFrame);

    transitionRayTracingImagePostCopy(commandBuffer, currentFrame);
  }

  void DynamicRenderer::transitionSwapchainImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
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

  void DynamicRenderer::transitionSwapchainImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                           const vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eNone,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::ePresentSrcKHR,
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

  void DynamicRenderer::transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                         const uint32_t currentFrame) const
  {
    const auto rtImage = getRayTracingImageResource(currentFrame)->getImage();
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

  void DynamicRenderer::transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const uint32_t currentFrame) const
  {
    const auto rtImage = getRayTracingImageResource(currentFrame)->getImage();
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
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
      },
      vk::ImageMemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferRead,
        .dstAccessMask = vk::AccessFlagBits::eNone,
        .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = rtImage,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
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

  void DynamicRenderer::copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                            const uint32_t currentFrame) const
  {
    const auto rtImage = getRayTracingImageResource(currentFrame)->getImage();
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