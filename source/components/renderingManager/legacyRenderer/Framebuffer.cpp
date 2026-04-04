#include "Framebuffer.h"
#include "RenderPass.h"
#include "../ImageResource.h"
#include "../RenderTarget.h"
#include "../../window/SwapChain.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {

  Framebuffer::Framebuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                           const std::shared_ptr<RenderTarget>& renderTarget,
                           const std::shared_ptr<RenderPass>& renderPass,
                           const vk::Extent2D extent,
                           const std::shared_ptr<SwapChain>& swapChain)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createFrameBuffers(renderPass->getRenderPass(), extent, renderTarget, swapChain);
  }

  const vk::raii::Framebuffer& Framebuffer::getFramebuffer(const uint32_t imageIndex) const
  {
    return m_framebuffers[imageIndex];
  }

  void Framebuffer::createFrameBuffers(const vk::raii::RenderPass& renderPass,
                                       const vk::Extent2D extent,
                                       const std::shared_ptr<RenderTarget>& renderTarget,
                                       const std::shared_ptr<SwapChain>& swapChain)
  {
    const auto numImages = swapChain ? swapChain->getImages().size() : m_logicalDevice->getMaxFramesInFlight();

    m_framebuffers.clear();
    m_framebuffers.reserve(numImages);

    for (size_t i = 0; i < numImages; i++)
    {
      std::vector<vk::ImageView> attachments;

      if (renderTarget->hasColorImageResource())
      {
        attachments.push_back(renderTarget->getColorImageResource(i).getImageView());
      }

      if (renderTarget->hasDepthImageResource())
      {
        attachments.push_back(renderTarget->getDepthImageResource(i).getImageView());
      }

      if (swapChain)
      {
        attachments.push_back(swapChain->getImageViews()[i]);
      }
      else if (renderTarget->hasResolveImageResource())
      {
        attachments.push_back(renderTarget->getResolveImageResource(i).getImageView());
      }

      const vk::FramebufferCreateInfo framebufferInfo {
        .renderPass = *renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1
      };

      m_framebuffers.push_back(m_logicalDevice->createFramebuffer(framebufferInfo));
    }
  }

} // namespace vke