#include "SwapchainFramebuffer.h"
#include "../SwapChain.h"

SwapchainFramebuffer::SwapchainFramebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const std::shared_ptr<SwapChain>& swapChain,
                                           const VkCommandPool& commandPool,
                                           const std::shared_ptr<RenderPass>& renderPass,
                                           const VkExtent2D extent)
  : Framebuffer(logicalDevice), m_swapChain(swapChain)
{
  initializeFramebuffer(commandPool, renderPass, extent);
}

VkFormat SwapchainFramebuffer::getColorFormat()
{
  return m_swapChain->getImageFormat();
}

const std::vector<VkImageView>& SwapchainFramebuffer::getImageViews()
{
  return m_swapChain->getImageViews();
}
