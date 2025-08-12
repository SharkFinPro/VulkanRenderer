#include "DynamicRenderer.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../core/physicalDevice/PhysicalDevice.h"
#include "../window/SwapChain.h"
#include "../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

DynamicRenderer::DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<SwapChain>& swapChain,
                                 VkCommandPool commandPool)
  : Renderer(logicalDevice, commandPool)
{
  createSampler();

  createSwapchainImageResources(swapChain);
}

DynamicRenderer::~DynamicRenderer()
{
  cleanupSwapchainImageResources();

  cleanupOffscreenImageResources();

  m_logicalDevice->destroySampler(m_sampler);
}

std::shared_ptr<RenderPass> DynamicRenderer::getRenderPass() const
{
  return nullptr;
}

VkDescriptorSet& DynamicRenderer::getOffscreenImageDescriptorSet(const uint32_t imageIndex)
{
  return m_offscreenImageDescriptorSets[imageIndex];
}

void DynamicRenderer::resetSwapchainImageResources(const std::shared_ptr<SwapChain> swapChain)
{
  cleanupSwapchainImageResources();
  createSwapchainImageResources(swapChain);
}

void DynamicRenderer::resetOffscreenImageResources(const VkExtent2D offscreenViewportExtent)
{
  cleanupOffscreenImageResources();
  createOffscreenImageResources(offscreenViewportExtent);
}

void DynamicRenderer::beginSwapchainRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                              const std::shared_ptr<CommandBuffer> commandBuffer,
                                              const std::shared_ptr<SwapChain> swapChain)
{
  VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_swapchainColorImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
    .resolveImageView = swapChain->getImageViews()[imageIndex],
    .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue = {
      .color = {0.0f, 0.0f, 0.0f, 1.0f}
    }
  };

  VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_swapchainDepthImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .clearValue = {
      .depthStencil = {1.0f, 0}
    }
  };

  const VkRenderingInfo renderingInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {
      .offset = {0, 0},
      .extent = extent,
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorRenderingAttachmentInfo,
    .pDepthAttachment = &depthRenderingAttachmentInfo,
  };

  commandBuffer->beginRendering(renderingInfo);
}

void DynamicRenderer::beginOffscreenRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                              const std::shared_ptr<CommandBuffer> commandBuffer)
{
  VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_offscreenColorImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
    .resolveImageView = m_offscreenImageViews[imageIndex],
    .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue = {
      .color = {0.0f, 0.0f, 0.0f, 1.0f}
    }
  };

  VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_offscreenDepthImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .clearValue = {
      .depthStencil = {1.0f, 0}
    }
  };

  const VkRenderingInfo renderingInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {
      .offset = {0, 0},
      .extent = extent,
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorRenderingAttachmentInfo,
    .pDepthAttachment = &depthRenderingAttachmentInfo,
  };

  commandBuffer->beginRendering(renderingInfo);
}

void DynamicRenderer::endSwapchainRendering(std::shared_ptr<CommandBuffer> commandBuffer,
                                            std::shared_ptr<SwapChain> swapChain)
{
  commandBuffer->endRendering();
}

void DynamicRenderer::endOffscreenRendering(std::shared_ptr<CommandBuffer> commandBuffer)
{
  commandBuffer->endRendering();
}

void DynamicRenderer::createSampler()
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

void DynamicRenderer::cleanupSwapchainImageResources()
{
  for (int i = 0; i < m_numImages; ++i)
  {
    m_logicalDevice->destroyImageView(m_swapchainColorImageViews[i]);
    m_logicalDevice->freeMemory(m_swapchainColorImageMemory[i]);
    m_logicalDevice->destroyImage(m_swapchainColorImages[i]);

    m_logicalDevice->destroyImageView(m_swapchainDepthImageViews[i]);
    m_logicalDevice->freeMemory(m_swapchainDepthImageMemory[i]);
    m_logicalDevice->destroyImage(m_swapchainDepthImages[i]);
  }
}

void DynamicRenderer::cleanupOffscreenImageResources()
{
  if (m_offscreenImages.empty())
  {
    return;
  }

  for (int i = 0; i < m_numImages; ++i)
  {
    ImGui_ImplVulkan_RemoveTexture(m_offscreenImageDescriptorSets[i]);

    m_logicalDevice->destroyImageView(m_offscreenImageViews[i]);
    m_logicalDevice->freeMemory(m_offscreenImageMemory[i]);
    m_logicalDevice->destroyImage(m_offscreenImages[i]);

    m_logicalDevice->destroyImageView(m_offscreenColorImageViews[i]);
    m_logicalDevice->freeMemory(m_offscreenColorImageMemory[i]);
    m_logicalDevice->destroyImage(m_offscreenColorImages[i]);

    m_logicalDevice->destroyImageView(m_offscreenDepthImageViews[i]);
    m_logicalDevice->freeMemory(m_offscreenDepthImageMemory[i]);
    m_logicalDevice->destroyImage(m_offscreenDepthImages[i]);
  }
}

void DynamicRenderer::createSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain)
{
  m_swapchainColorImageViews.resize(m_numImages);
  m_swapchainColorImageMemory.resize(m_numImages);
  m_swapchainColorImages.resize(m_numImages);

  m_swapchainDepthImageViews.resize(m_numImages);
  m_swapchainDepthImageMemory.resize(m_numImages);
  m_swapchainDepthImages.resize(m_numImages);

  for (int i = 0; i < m_numImages; ++i)
  {
    createColorImageResource(m_swapchainColorImages[i], m_swapchainColorImageViews[i],
                             m_swapchainColorImageMemory[i], swapChain->getImageFormat(), swapChain->getExtent());

    createDepthImageResource(m_swapchainDepthImages[i], m_swapchainDepthImageViews[i],
                             m_swapchainDepthImageMemory[i], swapChain->getExtent());
  }
}

void DynamicRenderer::createOffscreenImageResources(const VkExtent2D extent)
{
  m_offscreenImageDescriptorSets.resize(m_numImages);
  m_offscreenImageViews.resize(m_numImages);
  m_offscreenImageMemory.resize(m_numImages);
  m_offscreenImages.resize(m_numImages);

  m_offscreenColorImageViews.resize(m_numImages);
  m_offscreenColorImageMemory.resize(m_numImages);
  m_offscreenColorImages.resize(m_numImages);

  m_offscreenDepthImageViews.resize(m_numImages);
  m_offscreenDepthImageMemory.resize(m_numImages);
  m_offscreenDepthImages.resize(m_numImages);

  constexpr auto imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  for (int i = 0; i < m_numImages; ++i)
  {
    createImageResource(m_offscreenImages[i], m_offscreenImageViews[i], m_offscreenImageMemory[i],
                        m_offscreenImageDescriptorSets[i], extent);

    createColorImageResource(m_offscreenColorImages[i], m_offscreenColorImageViews[i],
                             m_offscreenColorImageMemory[i], imageFormat, extent);

    createDepthImageResource(m_offscreenDepthImages[i], m_offscreenDepthImageViews[i],
                             m_offscreenDepthImageMemory[i], extent);
  }
}

void DynamicRenderer::createColorImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory,
                                               const VkFormat format, const VkExtent2D extent) const
{
  const auto samples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples();

  Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1,
                      1, samples, format, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, VK_IMAGE_TYPE_2D, 1);

  imageView = Images::createImageView(m_logicalDevice, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                             VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(m_logicalDevice, m_commandPool, image, format, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);
}

void DynamicRenderer::createDepthImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory,
                                               const VkExtent2D extent) const
{
  const auto samples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples();

  const auto format = m_logicalDevice->getPhysicalDevice()->findDepthFormat();

  Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1, 1, samples,
                      format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, VK_IMAGE_TYPE_2D, 1);

  imageView = Images::createImageView(m_logicalDevice, image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                                      VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(m_logicalDevice, m_commandPool, image, format, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

void DynamicRenderer::createImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory,
                                          VkDescriptorSet& imageDescriptorSet, const VkExtent2D extent) const
{
  constexpr auto imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  Images::createImage(m_logicalDevice, 0, extent.width, extent.height, 1, 1,
                      VK_SAMPLE_COUNT_1_BIT, imageFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, VK_IMAGE_TYPE_2D, 1);

  imageView = Images::createImageView(m_logicalDevice, image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                      VK_IMAGE_VIEW_TYPE_2D, 1);

  Images::transitionImageLayout(m_logicalDevice, m_commandPool, image, imageFormat,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

  imageDescriptorSet = ImGui_ImplVulkan_AddTexture(m_sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
