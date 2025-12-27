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
    : Renderer(logicalDevice, swapChain, commandPool)
  {
    createRenderPasses(swapChain);

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

  std::shared_ptr<RenderPass> LegacyRenderer::getShadowRenderPass() const
  {
    return m_shadowRenderPass;
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

  void LegacyRenderer::createRenderPasses(const std::shared_ptr<SwapChain> &swapChain)
  {
    RenderPassConfig swapchainRenderPassConfig {
      .imageFormat = swapChain->getImageFormat(),
      .msaaSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .hasColorAttachment = true,
      .hasDepthAttachment = true,
      .hasResolveAttachment = true
    };
    m_swapchainRenderPass = std::make_shared<RenderPass>(m_logicalDevice, swapchainRenderPassConfig);

    RenderPassConfig offscreenRenderPassConfig {
      .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .msaaSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .hasColorAttachment = true,
      .hasDepthAttachment = true,
      .hasResolveAttachment = true
    };
    m_offscreenRenderPass = std::make_shared<RenderPass>(m_logicalDevice, offscreenRenderPassConfig);

    RenderPassConfig shadowRenderPassConfig {
      .imageFormat = VK_FORMAT_D32_SFLOAT,
      .msaaSamples = VK_SAMPLE_COUNT_1_BIT,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .hasColorAttachment = false,
      .hasDepthAttachment = true,
      .hasResolveAttachment = false
    };
    m_shadowRenderPass = std::make_shared<RenderPass>(m_logicalDevice, shadowRenderPassConfig);
  }
} // namespace vke