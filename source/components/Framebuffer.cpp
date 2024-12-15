#include "Framebuffer.h"
#include "../utilities/Images.h"
#include <array>
#include <utility>
#include <stdexcept>

Framebuffer::Framebuffer(std::shared_ptr<PhysicalDevice> physicalDevice,
                         std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<SwapChain> swapChain,
                         const VkCommandPool& commandPool,
                         const std::shared_ptr<RenderPass>& renderPass,
                         const bool presentToSwapChain)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice)),
    swapChain(std::move(swapChain)), presentToSwapChain(presentToSwapChain)
{
  if (!presentToSwapChain)
  {
    framebufferImageMemory.resize(3);
    framebufferImageViews.resize(3);
    framebufferImages.resize(3);

    for (int i = 0; i < 3; i++)
    {
      framebufferImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

      Images::createImage(this->logicalDevice->getDevice(), this->physicalDevice->getPhysicalDevice(), this->swapChain->getExtent().width, this->swapChain->getExtent().height,
                          1, VK_SAMPLE_COUNT_1_BIT, framebufferImageFormat, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          framebufferImages[i], framebufferImageMemory[i]);

      framebufferImageViews[i] = Images::createImageView(this->logicalDevice->getDevice(), framebufferImages[i], framebufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

      Images::transitionImageLayout(this->logicalDevice, commandPool, framebufferImages[i], framebufferImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }
  }

  createColorResources();
  createDepthResources(commandPool, renderPass->findDepthFormat());
  createFrameBuffers(renderPass->getRenderPass());
}

Framebuffer::~Framebuffer()
{
  // Destroy image views
  for (auto& imageView : framebufferImageViews) {
    if (imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(logicalDevice->getDevice(), imageView, nullptr);
      imageView = VK_NULL_HANDLE; // Nullify to avoid accidental reuse
    }
  }

  // Free image memory
  for (auto& imageMemory : framebufferImageMemory) {
    if (imageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(logicalDevice->getDevice(), imageMemory, nullptr);
      imageMemory = VK_NULL_HANDLE; // Nullify to avoid accidental reuse
    }
  }

  // Destroy images
  for (auto& image : framebufferImages) {
    if (image != VK_NULL_HANDLE) {
      vkDestroyImage(logicalDevice->getDevice(), image, nullptr);
      image = VK_NULL_HANDLE; // Nullify to avoid accidental reuse
    }
  }

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

VkFramebuffer& Framebuffer::getFramebuffer(const uint32_t imageIndex)
{
  return framebuffers[imageIndex];
}

void Framebuffer::createDepthResources(const VkCommandPool& commandPool, const VkFormat depthFormat)
{
  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getExtent().width, swapChain->getExtent().height,
                      1, physicalDevice->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      depthImage, depthImageMemory);
  depthImageView = Images::createImageView(logicalDevice->getDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  Images::transitionImageLayout(logicalDevice, commandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
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

void Framebuffer::createFrameBuffers(const VkRenderPass& renderPass)
{
  std::vector<VkImageView>* imageViews;

  if (presentToSwapChain)
  {
    imageViews = &swapChain->getImageViews();
  }
  else
  {
    imageViews = &framebufferImageViews;
  }

  framebuffers.resize(imageViews->size());

  for (size_t i = 0; i < imageViews->size(); i++)
  {
    std::array<VkImageView, 3> attachments {
      colorImageView,
      depthImageView,
      (*imageViews)[i]
    };

    const VkFramebufferCreateInfo framebufferInfo {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .width = swapChain->getExtent().width,
      .height = swapChain->getExtent().height,
      .layers = 1
    };

    if (vkCreateFramebuffer(logicalDevice->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}
