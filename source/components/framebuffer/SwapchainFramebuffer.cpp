#include "SwapchainFramebuffer.h"
#include "../SwapChain.h"

SwapchainFramebuffer::SwapchainFramebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const std::shared_ptr<SwapChain>& swapChain,
                                           const VkCommandPool& commandPool,
                                           const std::shared_ptr<RenderPass>& renderPass,
                                           const VkExtent2D extent)
  : Framebuffer(logicalDevice, commandPool, renderPass, extent), m_swapChain(swapChain)
{
}

VkFormat SwapchainFramebuffer::getColorFormat() const
{
  return m_swapChain->getImageFormat();
}

const std::vector<VkImageView>& SwapchainFramebuffer::getImageViews() const
{
  return m_swapChain->getImageViews();
}
