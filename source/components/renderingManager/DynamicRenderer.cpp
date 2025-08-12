#include "DynamicRenderer.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include <backends/imgui_impl_vulkan.h>

DynamicRenderer::DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Renderer(logicalDevice)
{}

DynamicRenderer::~DynamicRenderer()
{
  cleanupSwapchainImageResources();

  cleanupOffscreenImageResources();
}

std::shared_ptr<RenderPass> DynamicRenderer::getRenderPass() const
{
  return nullptr;
}

VkDescriptorSet& DynamicRenderer::getOffscreenImageDescriptorSet(const uint32_t imageIndex) const
{
}

void DynamicRenderer::resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain)
{
}

void DynamicRenderer::resetOffscreenImageResources(const VkExtent2D offscreenViewportExtent)
{
}

void DynamicRenderer::beginSwapchainRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                              const std::shared_ptr<CommandBuffer> commandBuffer)
{
  const VkRenderingInfo renderingInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {
      .offset = {0, 0},
      .extent = extent,
    }
  };

  commandBuffer->beginRendering(renderingInfo);
}

void DynamicRenderer::beginOffscreenRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                              const std::shared_ptr<CommandBuffer> commandBuffer)
{
  VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_offscreenColorImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue {
      .color = {0.0f, 0.0f, 0.0f, 1.0f},
      .depthStencil = {1.0f, 0}
    }
  };

  VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = m_offscreenDepthImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .clearValue {
      .color = {0.0f, 0.0f, 0.0f, 1.0f},
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

void DynamicRenderer::endRendering(std::shared_ptr<CommandBuffer> commandBuffer)
{
  commandBuffer->endRendering();
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
