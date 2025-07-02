#include "Framebuffer.h"
#include "../core/physicalDevice/PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "../pipelines/RenderPass.h"
#include "../utilities/Images.h"
#include <utility>
#include <stdexcept>
#include <backends/imgui_impl_vulkan.h>

Framebuffer::Framebuffer(std::shared_ptr<PhysicalDevice> physicalDevice,
                         std::shared_ptr<LogicalDevice> logicalDevice,
                         std::shared_ptr<SwapChain> swapChain,
                         const VkCommandPool& commandPool,
                         const std::shared_ptr<RenderPass>& renderPass,
                         const VkExtent2D extent,
                         const bool mousePicking)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice)),
    swapChain(std::move(swapChain)), m_mousePicking(mousePicking)
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
    logicalDevice->destroySampler(sampler);

    for (const auto& framebufferImageDescriptorSet : framebufferImageDescriptorSets)
    {
      ImGui_ImplVulkan_RemoveTexture(framebufferImageDescriptorSet);
    }
  }

  for (auto& imageView : framebufferImageViews)
  {
    logicalDevice->destroyImageView(imageView);
  }

  for (auto& imageMemory : framebufferImageMemory)
  {
    logicalDevice->freeMemory(imageMemory);
  }

  for (auto& image : framebufferImages)
  {
    logicalDevice->destroyImage(image);
  }

  logicalDevice->destroyImageView(colorImageView);
  logicalDevice->destroyImage(colorImage);
  logicalDevice->freeMemory(colorImageMemory);

  logicalDevice->destroyImageView(depthImageView);
  logicalDevice->destroyImage(depthImage);
  logicalDevice->freeMemory(depthImageMemory);

  for (auto& framebuffer : framebuffers)
  {
    logicalDevice->destroyFramebuffer(framebuffer);
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

VkImage& Framebuffer::getColorImage()
{
  return colorImage;
}

void Framebuffer::createImageResources(const VkCommandPool& commandPool, const VkExtent2D extent)
{
  if (swapChain)
  {
    return;
  }

  framebufferImageFormat = m_mousePicking ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;

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

  sampler = logicalDevice->createSampler(samplerInfo);

  constexpr size_t numImages = 3;
  framebufferImageMemory.resize(numImages);
  framebufferImageViews.resize(numImages);
  framebufferImages.resize(numImages);
  framebufferImageDescriptorSets.resize(numImages);

  for (int i = 0; i < numImages; i++)
  {
    Images::createImage(logicalDevice, physicalDevice, 0, extent.width, extent.height, 1,
                        1, VK_SAMPLE_COUNT_1_BIT, framebufferImageFormat, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        framebufferImages[i], framebufferImageMemory[i], VK_IMAGE_TYPE_2D, 1);

    framebufferImageViews[i] = Images::createImageView(logicalDevice, framebufferImages[i],
                                                       framebufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                                       VK_IMAGE_VIEW_TYPE_2D, 1);

    Images::transitionImageLayout(this->logicalDevice, commandPool, framebufferImages[i], framebufferImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

    framebufferImageDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(sampler, framebufferImageViews[i],
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void Framebuffer::createDepthResources(const VkCommandPool& commandPool, const VkFormat depthFormat, const VkExtent2D extent)
{
  const VkSampleCountFlagBits samples = m_mousePicking ? VK_SAMPLE_COUNT_1_BIT : physicalDevice->getMsaaSamples();

  Images::createImage(logicalDevice, physicalDevice, 0, extent.width, extent.height, 1,
                      1, samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      depthImage, depthImageMemory, VK_IMAGE_TYPE_2D, 1);

  depthImageView = Images::createImageView(logicalDevice, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                                           VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(logicalDevice, commandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

void Framebuffer::createColorResources(const VkExtent2D extent)
{
  const VkSampleCountFlagBits samples = m_mousePicking ? VK_SAMPLE_COUNT_1_BIT : physicalDevice->getMsaaSamples();

  const VkFormat colorFormat = swapChain ? swapChain->getImageFormat() : framebufferImageFormat;

  Images::createImage(logicalDevice, physicalDevice, 0, extent.width, extent.height, 1,
                      1, samples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      (m_mousePicking ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT),
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory, VK_IMAGE_TYPE_2D, 1);

  colorImageView = Images::createImageView(logicalDevice, colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                           VK_IMAGE_VIEW_TYPE_2D, 1);
}

void Framebuffer::createFrameBuffers(const VkRenderPass& renderPass, const VkExtent2D extent)
{
  const std::vector<VkImageView>& imageViews = swapChain ? swapChain->getImageViews() : framebufferImageViews;

  framebuffers.resize(imageViews.size());

  for (size_t i = 0; i < imageViews.size(); i++)
  {
    std::vector<VkImageView> attachments {
      colorImageView,
      depthImageView
    };

    if (!m_mousePicking)
    {
      attachments.push_back(imageViews.at(i));
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

    framebuffers[i] = logicalDevice->createFramebuffer(framebufferInfo);
  }
}
