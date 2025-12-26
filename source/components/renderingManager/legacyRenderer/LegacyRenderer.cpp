#include "LegacyRenderer.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../window/SwapChain.h"

namespace vke {

  LegacyRenderer::LegacyRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<SwapChain>& swapChain,
                                 VkCommandPool commandPool)
    : Renderer(logicalDevice, swapChain, commandPool),
      m_swapchainRenderPass(std::make_shared<RenderPass>(m_logicalDevice, swapChain->getImageFormat(),
                                                         m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
                                                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)),
      m_offscreenRenderPass(std::make_shared<RenderPass>(m_logicalDevice, VK_FORMAT_B8G8R8A8_UNORM,
                                                         m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
  {
    m_swapchainFramebuffer = std::make_shared<Framebuffer>(
      m_logicalDevice,
      m_swapchainRenderTarget,
      m_swapchainRenderPass,
      swapChain->getExtent(),
      swapChain
    );
  }

  std::shared_ptr<RenderPass> LegacyRenderer::getSwapchainRenderPass() const
  {
    return m_swapchainRenderPass;
  }

  std::shared_ptr<RenderPass> LegacyRenderer::getOffscreenRenderPass() const
  {
    return m_offscreenRenderPass;
  }

  void LegacyRenderer::resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain)
  {
    m_swapchainFramebuffer.reset();

    Renderer::resetSwapchainImageResources(swapChain);

    m_swapchainFramebuffer = std::make_shared<Framebuffer>(
      m_logicalDevice,
      m_swapchainRenderTarget,
      m_swapchainRenderPass,
      swapChain->getExtent(),
      swapChain
    );
  }

  void LegacyRenderer::resetOffscreenImageResources(const VkExtent2D offscreenViewportExtent)
  {
    m_offscreenFramebuffer.reset();

    Renderer::resetOffscreenImageResources(offscreenViewportExtent);

    m_offscreenFramebuffer = std::make_shared<Framebuffer>(
      m_logicalDevice,
      m_offscreenRenderTarget,
      m_offscreenRenderPass,
      offscreenViewportExtent
    );
  }

  void LegacyRenderer::beginSwapchainRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                               const std::shared_ptr<CommandBuffer> commandBuffer,
                                               [[maybe_unused]] std::shared_ptr<SwapChain> swapChain)
  {
    m_swapchainRenderPass->begin(m_swapchainFramebuffer->getFramebuffer(imageIndex), extent, commandBuffer);
  }

  void LegacyRenderer::beginOffscreenRendering(const uint32_t imageIndex, const VkExtent2D extent,
                                               const std::shared_ptr<CommandBuffer> commandBuffer)
  {
    m_offscreenRenderPass->begin(m_offscreenFramebuffer->getFramebuffer(imageIndex), extent, commandBuffer);
  }

  void LegacyRenderer::beginShadowRendering(uint32_t imageIndex,
                                            VkExtent2D extent,
                                            const std::shared_ptr<CommandBuffer>& commandBuffer,
                                            const std::shared_ptr<Light>& light)
  {
    // m_shadowRenderPass->begin(m_shadowFramebuffer->getFramebuffer(imageIndex), extent, commandBuffer);
  }

  void LegacyRenderer::endSwapchainRendering(uint32_t imageIndex,
                                             const std::shared_ptr<CommandBuffer> commandBuffer,
                                             [[maybe_unused]] std::shared_ptr<SwapChain> swapChain)
  {
    endRendering(commandBuffer);
  }

  void LegacyRenderer::endOffscreenRendering(uint32_t imageIndex,
                                             const std::shared_ptr<CommandBuffer> commandBuffer)
  {
    endRendering(commandBuffer);
  }

  void LegacyRenderer::endShadowRendering(uint32_t imageIndex,
                                          const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    // endRendering(commandBuffer);
  }

  void LegacyRenderer::endRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    commandBuffer->endRenderPass();
  }
} // namespace vke