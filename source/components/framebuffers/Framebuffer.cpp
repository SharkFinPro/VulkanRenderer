#include "Framebuffer.h"
#include "../window/SwapChain.h"
#include "../../components/logicalDevice/LogicalDevice.h"
#include "../../components/physicalDevice/PhysicalDevice.h"
#include "../renderPass/RenderPass.h"
#include "../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {

Framebuffer::Framebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const bool mousePicking)
  : m_logicalDevice(logicalDevice), m_mousePicking(mousePicking)
{}

Framebuffer::~Framebuffer()
{
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

void Framebuffer::initializeFramebuffer(const VkCommandPool& commandPool,
                                        const std::shared_ptr<RenderPass>& renderPass,
                                        const VkExtent2D extent)
{
  createColorResources(extent);

  createDepthResources(commandPool, m_logicalDevice->getPhysicalDevice()->findDepthFormat(), extent);

  createFrameBuffers(renderPass->getRenderPass(), extent);
}

VkFramebuffer& Framebuffer::getFramebuffer(const uint32_t imageIndex)
{
  return m_framebuffers[imageIndex];
}

VkImage& Framebuffer::getColorImage()
{
  return m_colorImage;
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

} // namespace vke