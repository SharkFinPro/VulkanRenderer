#include "Framebuffer.h"
#include "../utilities/Images.h"
#include <array>
#include <utility>
#include <stdexcept>
#include <backends/imgui_impl_vulkan.h>

Framebuffer::Framebuffer(std::shared_ptr<PhysicalDevice> physicalDevice,
                         std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<SwapChain> swapChain,
                         const VkCommandPool& commandPool,
                         const std::shared_ptr<RenderPass>& renderPass,
                         const VkExtent2D extent)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice)),
    swapChain(std::move(swapChain))
{
  createImageResources(commandPool, extent);

  createColorResources(extent);

  createDepthResources(commandPool, renderPass->findDepthFormat(), extent);

  createFrameBuffers(renderPass->getRenderPass(), extent);
}

Framebuffer::~Framebuffer()
{
  if (!swapChain)
  {
    vkDestroySampler(logicalDevice->getDevice(), sampler, nullptr);

    for (const auto& framebufferImageDescriptorSet : framebufferImageDescriptorSets)
    {
      ImGui_ImplVulkan_RemoveTexture(framebufferImageDescriptorSet);
    }
  }

  for (const auto& imageView : framebufferImageViews)
  {
    vkDestroyImageView(logicalDevice->getDevice(), imageView, nullptr);
  }

  for (const auto& imageMemory : framebufferImageMemory)
  {
    vkFreeMemory(logicalDevice->getDevice(), imageMemory, nullptr);
  }

  for (const auto& image : framebufferImages)
  {
    vkDestroyImage(logicalDevice->getDevice(), image, nullptr);
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

VkDescriptorSet& Framebuffer::getFramebufferImageDescriptorSet(const uint32_t imageIndex)
{
  return framebufferImageDescriptorSets[imageIndex];
}

void Framebuffer::createImageResources(const VkCommandPool& commandPool, const VkExtent2D extent)
{
  if (swapChain)
  {
    return;
  }

  framebufferImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  constexpr VkSamplerCreateInfo samplerInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 1.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };

  if (vkCreateSampler(this->logicalDevice->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create image sampler!");
  }

  constexpr size_t numImages = 3;
  framebufferImageMemory.resize(numImages);
  framebufferImageViews.resize(numImages);
  framebufferImages.resize(numImages);
  framebufferImageDescriptorSets.resize(numImages);

  for (int i = 0; i < numImages; i++)
  {
    Images::createImage(this->logicalDevice->getDevice(), this->physicalDevice->getPhysicalDevice(), extent.width, extent.height, 1,
                        1, VK_SAMPLE_COUNT_1_BIT, framebufferImageFormat, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        framebufferImages[i], framebufferImageMemory[i], VK_IMAGE_TYPE_2D);

    framebufferImageViews[i] = Images::createImageView(this->logicalDevice->getDevice(), framebufferImages[i],
                                                       framebufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D);

    Images::transitionImageLayout(this->logicalDevice, commandPool, framebufferImages[i], framebufferImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    framebufferImageDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(sampler, framebufferImageViews[i],
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void Framebuffer::createDepthResources(const VkCommandPool& commandPool, const VkFormat depthFormat, const VkExtent2D extent)
{
  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), extent.width, extent.height, 1,
                      1, physicalDevice->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      depthImage, depthImageMemory, VK_IMAGE_TYPE_2D);
  depthImageView = Images::createImageView(logicalDevice->getDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_IMAGE_VIEW_TYPE_2D);

  Images::transitionImageLayout(logicalDevice, commandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Framebuffer::createColorResources(const VkExtent2D extent)
{
  const VkFormat colorFormat = swapChain ? swapChain->getImageFormat() : framebufferImageFormat;

  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), extent.width, extent.height, 1,
                      1, physicalDevice->getMsaaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory, VK_IMAGE_TYPE_2D);

  colorImageView = Images::createImageView(logicalDevice->getDevice(), colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D);
}

void Framebuffer::createFrameBuffers(const VkRenderPass& renderPass, const VkExtent2D extent)
{
  const std::vector<VkImageView>& imageViews = swapChain ? swapChain->getImageViews() : framebufferImageViews;

  framebuffers.resize(imageViews.size());

  for (size_t i = 0; i < imageViews.size(); i++)
  {
    std::array<VkImageView, 3> attachments {
      colorImageView,
      depthImageView,
      imageViews.at(i)
    };

    const VkFramebufferCreateInfo framebufferInfo {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .width = extent.width,
      .height = extent.height,
      .layers = 1
    };

    if (vkCreateFramebuffer(logicalDevice->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}
