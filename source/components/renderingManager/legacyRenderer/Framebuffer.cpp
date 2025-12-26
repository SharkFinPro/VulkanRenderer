#include "Framebuffer.h"
#include "RenderPass.h"
#include "../ImageResource.h"
#include "../RenderTarget.h"
#include "../../window/SwapChain.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {

  Framebuffer::Framebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<RenderTarget>& renderTarget,
                           const std::shared_ptr<RenderPass>& renderPass,
                           const VkExtent2D extent,
                           const std::shared_ptr<SwapChain>& swapChain)
    : m_logicalDevice(logicalDevice)
  {
    createFrameBuffers(renderPass->getRenderPass(), extent, renderTarget, swapChain);
  }

  Framebuffer::~Framebuffer()
  {
    for (auto& framebuffer : m_framebuffers)
    {
      m_logicalDevice->destroyFramebuffer(framebuffer);
    }
  }

  VkFramebuffer& Framebuffer::getFramebuffer(const uint32_t imageIndex)
  {
    return m_framebuffers[imageIndex];
  }

  void Framebuffer::createFrameBuffers(const VkRenderPass& renderPass,
                                       const VkExtent2D extent,
                                       const std::shared_ptr<RenderTarget>& renderTarget,
                                       const std::shared_ptr<SwapChain>& swapChain)
  {
    m_framebuffers.resize(NUM_IMAGES);

    for (size_t i = 0; i < NUM_IMAGES; i++)
    {
      std::vector<VkImageView> attachments;

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

      const VkFramebufferCreateInfo framebufferInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1
      };

      m_framebuffers[i] = m_logicalDevice->createFramebuffer(framebufferInfo);
    }
  }

} // namespace vke