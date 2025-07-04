#include "StandardFramebuffer.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

StandardFramebuffer::StandardFramebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         const VkCommandPool& commandPool,
                                         const std::shared_ptr<RenderPass>& renderPass,
                                         const VkExtent2D extent,
                                         const bool mousePicking)
  : Framebuffer(logicalDevice, mousePicking), m_commandPool(commandPool), m_extent(extent)
{
  createSampler();

  createImageResources();

  initializeFramebuffer(commandPool, renderPass, extent);
}

StandardFramebuffer::~StandardFramebuffer()
{
  m_logicalDevice->destroySampler(m_sampler);

  for (const auto& framebufferImageDescriptorSet : m_framebufferImageDescriptorSets)
  {
    ImGui_ImplVulkan_RemoveTexture(framebufferImageDescriptorSet);
  }

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
}

VkDescriptorSet& StandardFramebuffer::getFramebufferImageDescriptorSet(const uint32_t imageIndex)
{
  return m_framebufferImageDescriptorSets[imageIndex];
}

VkFormat StandardFramebuffer::getColorFormat()
{
  return m_framebufferImageFormat;
}

const std::vector<VkImageView>& StandardFramebuffer::getImageViews()
{
  return m_framebufferImageViews;
}

void StandardFramebuffer::createSampler()
{
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
}

void StandardFramebuffer::createImageResources()
{
  m_framebufferImageFormat = m_mousePicking ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;

  constexpr size_t numImages = 3;
  m_framebufferImageMemory.resize(numImages);
  m_framebufferImageViews.resize(numImages);
  m_framebufferImages.resize(numImages);
  m_framebufferImageDescriptorSets.resize(numImages);

  for (int i = 0; i < numImages; i++)
  {
    createImageResource(i);
  }
}

void StandardFramebuffer::createImageResource(const size_t imageIndex)
{
  Images::createImage(m_logicalDevice, 0, m_extent.width, m_extent.height, 1,
                      1, VK_SAMPLE_COUNT_1_BIT, m_framebufferImageFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_framebufferImages[imageIndex],
                      m_framebufferImageMemory[imageIndex], VK_IMAGE_TYPE_2D, 1);

  m_framebufferImageViews[imageIndex] = Images::createImageView(m_logicalDevice, m_framebufferImages[imageIndex],
                                                                m_framebufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT,
                                                                1, VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(m_logicalDevice, m_commandPool, m_framebufferImages[imageIndex],
                                m_framebufferImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

  m_framebufferImageDescriptorSets[imageIndex] = ImGui_ImplVulkan_AddTexture(m_sampler,
                                                                             m_framebufferImageViews[imageIndex],
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
