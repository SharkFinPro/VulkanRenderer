#include "LegacyRenderer.h"
#include "framebuffers/StandardFramebuffer.h"
#include "framebuffers/SwapchainFramebuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../window/SwapChain.h"
#include "../../renderPass/RenderPass.h"

namespace vke {

LegacyRenderer::LegacyRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const std::shared_ptr<SwapChain>& swapChain,
                               VkCommandPool commandPool)
  : Renderer(logicalDevice, commandPool),
    m_renderPass(std::make_shared<RenderPass>(m_logicalDevice, swapChain->getImageFormat(),
                                              m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)),
    m_offscreenRenderPass(std::make_shared<RenderPass>(m_logicalDevice, VK_FORMAT_B8G8R8A8_UNORM,
                                                       m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
{
  m_framebuffer = std::make_shared<SwapchainFramebuffer>(m_logicalDevice, swapChain, m_commandPool, m_renderPass,
                                                         swapChain->getExtent());
}

std::shared_ptr<RenderPass> LegacyRenderer::getRenderPass() const
{
  return m_renderPass;
}

VkDescriptorSet& LegacyRenderer::getOffscreenImageDescriptorSet(const uint32_t imageIndex)
{
  return m_offscreenFramebuffer->getFramebufferImageDescriptorSet(imageIndex);
}

void LegacyRenderer::resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain)
{
  m_framebuffer.reset();
  m_framebuffer = std::make_shared<SwapchainFramebuffer>(m_logicalDevice, swapChain, m_commandPool, m_renderPass,
                                                         swapChain->getExtent());
}

void LegacyRenderer::resetOffscreenImageResources(const VkExtent2D offscreenViewportExtent)
{
  m_offscreenFramebuffer.reset();
  m_offscreenFramebuffer = std::make_shared<StandardFramebuffer>(m_logicalDevice, m_commandPool, m_renderPass,
                                                                 offscreenViewportExtent);
}

void LegacyRenderer::beginSwapchainRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                             const std::shared_ptr<CommandBuffer> commandBuffer,
                                             [[maybe_unused]] std::shared_ptr<SwapChain> swapChain)
{
  m_renderPass->begin(m_framebuffer->getFramebuffer(imageIndex), extent, commandBuffer);
}

void LegacyRenderer::beginOffscreenRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                             const std::shared_ptr<CommandBuffer> commandBuffer)
{
  m_offscreenRenderPass->begin(m_offscreenFramebuffer->getFramebuffer(imageIndex), extent, commandBuffer);
}

void LegacyRenderer::endSwapchainRendering(uint32_t imageIndex, const std::shared_ptr<CommandBuffer> commandBuffer,
                                           [[maybe_unused]] std::shared_ptr<SwapChain> swapChain)
{
  endRendering(commandBuffer);
}

void LegacyRenderer::endOffscreenRendering(uint32_t imageIndex, const std::shared_ptr<CommandBuffer> commandBuffer)
{
  endRendering(commandBuffer);
}

void LegacyRenderer::endRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
{
  commandBuffer->endRenderPass();
}

} // namespace vke