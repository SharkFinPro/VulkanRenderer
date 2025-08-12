#include "DynamicRenderer.h"
#include "../core/commandBuffer/CommandBuffer.h"

DynamicRenderer::DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : Renderer(logicalDevice)
{}

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
                                              std::shared_ptr<CommandBuffer> commandBuffer)
{
  VkRenderingInfo renderingInfo {};

  commandBuffer->beginRendering(renderingInfo);
}

void DynamicRenderer::beginOffscreenRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                              std::shared_ptr<CommandBuffer> commandBuffer)
{
  VkRenderingInfo renderingInfo {};

  commandBuffer->beginRendering(renderingInfo);
}

void DynamicRenderer::endRendering(std::shared_ptr<CommandBuffer> commandBuffer)
{
  commandBuffer->endRendering();
}
