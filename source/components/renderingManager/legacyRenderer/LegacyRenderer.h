#ifndef VKE_LEGACYRENDERER_H
#define VKE_LEGACYRENDERER_H

#include "../Renderer.h"
#include <memory>
#include <unordered_map>

namespace vke {

  class Framebuffer;
  class RenderPass;
  class SwapChain;

  class LegacyRenderer final : public Renderer {
  public:
    LegacyRenderer(std::shared_ptr<LogicalDevice> logicalDevice,
                   const std::shared_ptr<SwapChain>& swapChain,
                   VkCommandPool commandPool);

    [[nodiscard]] std::shared_ptr<RenderPass> getSwapchainRenderPass() const override;

    [[nodiscard]] std::shared_ptr<RenderPass> getOffscreenRenderPass() const override;

    [[nodiscard]] std::shared_ptr<RenderPass> getShadowRenderPass() const override;

    [[nodiscard]] std::shared_ptr<RenderPass> getShadowCubeRenderPass() const override;

    [[nodiscard]] std::shared_ptr<RenderPass> getMousePickingRenderPass() const override;

    void resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain) override;

    void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) override;

    void resetMousePickingImageResources(VkExtent2D mousePickingExtent) override;

    void beginSwapchainRendering(uint32_t imageIndex,
                                 VkExtent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer,
                                 std::shared_ptr<SwapChain> swapChain) override;

    void beginOffscreenRendering(uint32_t imageIndex,
                                 VkExtent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer) override;

    void beginShadowRendering(uint32_t imageIndex,
                              VkExtent2D extent,
                              const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const std::shared_ptr<Light>& light) override;

    void beginMousePickingRendering(uint32_t imageIndex,
                                    VkExtent2D extent,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    void endSwapchainRendering(uint32_t imageIndex,
                               std::shared_ptr<CommandBuffer> commandBuffer,
                               std::shared_ptr<SwapChain> swapChain) override;

    void endOffscreenRendering(std::shared_ptr<CommandBuffer> commandBuffer) override;

    void endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    void endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    [[nodiscard]] uint32_t registerShadowMapRenderTarget(std::shared_ptr<RenderTarget> renderTarget,
                                                         bool isCubeMap) override;

  private:
    std::shared_ptr<Framebuffer> m_swapchainFramebuffer;
    std::shared_ptr<Framebuffer> m_offscreenFramebuffer;
    std::shared_ptr<Framebuffer> m_mousePickingFramebuffer;

    std::shared_ptr<RenderPass> m_swapchainRenderPass;
    std::shared_ptr<RenderPass> m_offscreenRenderPass;
    std::shared_ptr<RenderPass> m_shadowRenderPass;
    std::shared_ptr<RenderPass> m_shadowCubeRenderPass;
    std::shared_ptr<RenderPass> m_mousePickingRenderPass;

    std::unordered_map<uint32_t, std::shared_ptr<Framebuffer>> m_shadowMapFramebuffers;

    static void endRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);

    void createRenderPasses(const std::shared_ptr<SwapChain>& swapChain);
  };

} // namespace vke

#endif //VKE_LEGACYRENDERER_H
