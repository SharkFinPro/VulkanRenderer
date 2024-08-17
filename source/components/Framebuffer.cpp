#include "Framebuffer.h"
#include "../utilities/Images.h"
#include <array>
#include <utility>
#include <stdexcept>

Framebuffer::Framebuffer(std::shared_ptr<PhysicalDevice> physicalDevice,
                         std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<SwapChain> swapChain,
                         VkCommandPool& commandPool,
                         std::shared_ptr<RenderPass> renderPass)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice)), swapChain(std::move(swapChain))
{
  createColorResources();
  createDepthResources(commandPool, renderPass->findDepthFormat());
  createFrameBuffers(renderPass->getRenderPass());
}

Framebuffer::~Framebuffer()
{
  vkDestroyImageView(logicalDevice->getDevice(), colorImageView, nullptr);
  vkDestroyImage(logicalDevice->getDevice(), colorImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), colorImageMemory, nullptr);

  vkDestroyImageView(logicalDevice->getDevice(), depthImageView, nullptr);
  vkDestroyImage(logicalDevice->getDevice(), depthImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), depthImageMemory, nullptr);

  for (const auto framebuffer : framebuffers)
  {
    vkDestroyFramebuffer(logicalDevice->getDevice(), framebuffer, nullptr);
  }
}

VkFramebuffer& Framebuffer::getFramebuffer(uint32_t imageIndex)
{
  return framebuffers[imageIndex];
}

void Framebuffer::createDepthResources(VkCommandPool& commandPool, VkFormat depthFormat)
{
  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getExtent().width, swapChain->getExtent().height,
                      1, physicalDevice->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      depthImage, depthImageMemory);
  depthImageView = Images::createImageView(logicalDevice->getDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  Images::transitionImageLayout(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(),
                                depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Framebuffer::createColorResources()
{
  const VkFormat colorFormat = swapChain->getImageFormat();

  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getExtent().width, swapChain->getExtent().height,
                      1, physicalDevice->getMsaaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

  colorImageView = Images::createImageView(logicalDevice->getDevice(), colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Framebuffer::createFrameBuffers(VkRenderPass& renderPass)
{
  const auto swapChainImageViews = swapChain->getImageViews();

  framebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++)
  {
    std::array<VkImageView, 3> attachments = {
      colorImageView,
      depthImageView,
      swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChain->getExtent().width;
    framebufferInfo.height = swapChain->getExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}
