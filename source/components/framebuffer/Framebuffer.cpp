#include "Framebuffer.h"
#include "../SwapChain.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../pipelines/RenderPass.h"
#include "../../utilities/Images.h"
#include <stdexcept>
#include <backends/imgui_impl_vulkan.h>

Framebuffer::Framebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const VkCommandPool& commandPool,
                         const std::shared_ptr<RenderPass>& renderPass,
                         const VkExtent2D extent,
                         const bool mousePicking)
  : m_logicalDevice(logicalDevice), m_mousePicking(mousePicking)
{
  createImageResources(commandPool, extent);

  createColorResources(extent);

  createDepthResources(commandPool, renderPass->findDepthFormat(), extent);

  createFrameBuffers(renderPass->getRenderPass(), extent);
}

Framebuffer::~Framebuffer()
{
  // TODO: Child Pipeline
  // if (!m_swapChain)
  // {
  //   m_logicalDevice->destroySampler(m_sampler);
  //
  //   for (const auto& framebufferImageDescriptorSet : m_framebufferImageDescriptorSets)
  //   {
  //     ImGui_ImplVulkan_RemoveTexture(framebufferImageDescriptorSet);
  //   }
  // }

  for (auto& imageView : m_framebufferImageViews)
  {
    m_logicalDevice->destroyImageView(imageView);
  }

  for (auto& imageMemory : m_framebufferImageMemory)
  {
    m_logicalDevice->freeMemory(imageMemory);
  }

  for (auto& image : m_framebufferImages)
  {
    m_logicalDevice->destroyImage(image);
  }

  m_logicalDevice->destroyImageView(m_colorImageView);
  m_logicalDevice->destroyImage(m_colorImage);
  m_logicalDevice->freeMemory(m_colorImageMemory);

  m_logicalDevice->destroyImageView(m_depthImageView);
  m_logicalDevice->destroyImage(m_depthImage);
  m_logicalDevice->freeMemory(m_depthImageMemory);

  for (auto& framebuffer : m_framebuffers)
  {
    m_logicalDevice->destroyFramebuffer(framebuffer);
  }
}

VkFramebuffer& Framebuffer::getFramebuffer(const uint32_t imageIndex)
{
  return m_framebuffers[imageIndex];
}

VkDescriptorSet& Framebuffer::getFramebufferImageDescriptorSet(const uint32_t imageIndex)
{
  return m_framebufferImageDescriptorSets[imageIndex];
}

VkImage& Framebuffer::getColorImage()
{
  return m_colorImage;
}

void Framebuffer::createImageResources(const VkCommandPool& commandPool, const VkExtent2D extent)
{
  // TODO: Child Pipeline
  // if (m_swapChain)
  // {
    return;
  // }

  m_framebufferImageFormat = m_mousePicking ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;

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

  m_sampler = m_logicalDevice->createSampler(samplerInfo);

  constexpr size_t numImages = 3;
  m_framebufferImageMemory.resize(numImages);
  m_framebufferImageViews.resize(numImages);
  m_framebufferImages.resize(numImages);
  m_framebufferImageDescriptorSets.resize(numImages);

  for (int i = 0; i < numImages; i++)
  {
    Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1,
                        1, VK_SAMPLE_COUNT_1_BIT, m_framebufferImageFormat, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        m_framebufferImages[i], m_framebufferImageMemory[i], VK_IMAGE_TYPE_2D, 1);

    m_framebufferImageViews[i] = Images::createImageView(m_logicalDevice, m_framebufferImages[i],
                                                         m_framebufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                                         VK_IMAGE_VIEW_TYPE_2D, 1);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_framebufferImages[i], m_framebufferImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

    m_framebufferImageDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_sampler, m_framebufferImageViews[i],
                                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void Framebuffer::createDepthResources(const VkCommandPool& commandPool, const VkFormat depthFormat, const VkExtent2D extent)
{
  const VkSampleCountFlagBits samples = m_mousePicking ? VK_SAMPLE_COUNT_1_BIT : m_logicalDevice->getPhysicalDevice()->getMsaaSamples();

  Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1,
                      1, samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      m_depthImage, m_depthImageMemory, VK_IMAGE_TYPE_2D, 1);

  m_depthImageView = Images::createImageView(m_logicalDevice, m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                                             VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(m_logicalDevice, commandPool, m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

void Framebuffer::createColorResources(const VkExtent2D extent)
{
  const VkSampleCountFlagBits samples = m_mousePicking ? VK_SAMPLE_COUNT_1_BIT : m_logicalDevice->getPhysicalDevice()->getMsaaSamples();

  const auto colorFormat = getColorFormat();

  Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1,
                      1, samples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      (m_mousePicking ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT),
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory, VK_IMAGE_TYPE_2D, 1);

  m_colorImageView = Images::createImageView(m_logicalDevice, m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                             VK_IMAGE_VIEW_TYPE_2D, 1);
}

void Framebuffer::createFrameBuffers(const VkRenderPass& renderPass, const VkExtent2D extent)
{
  const auto imageViews = getImageViews();

  m_framebuffers.resize(imageViews.size());

  for (size_t i = 0; i < imageViews.size(); i++)
  {
    std::vector<VkImageView> attachments {
      m_colorImageView,
      m_depthImageView
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

    m_framebuffers[i] = m_logicalDevice->createFramebuffer(framebufferInfo);
  }
}
