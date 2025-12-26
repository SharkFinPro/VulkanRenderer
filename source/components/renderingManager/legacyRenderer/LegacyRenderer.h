#ifndef VKE_LEGACYRENDERER_H
#define VKE_LEGACYRENDERER_H

#include "../Renderer.h"
#include <memory>

namespace vke {

  class Framebuffer;
  class RenderPass;
  class SwapChain;

  class LegacyRenderer final : public Renderer {
  public:
    LegacyRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice, const std::shared_ptr<SwapChain>& swapChain,
                   VkCommandPool commandPool);

    [[nodiscard]] std::shared_ptr<RenderPass> getSwapchainRenderPass() const override;

    [[nodiscard]] std::shared_ptr<RenderPass> getOffscreenRenderPass() const override;

    void resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain) override;

    void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) override;

    void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer,
                                 std::shared_ptr<SwapChain> swapChain) override;

    void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer) override;

    void beginShadowRendering(uint32_t imageIndex,
                              VkExtent2D extent,
                              const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const std::shared_ptr<Light>& light) override;

    void endSwapchainRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer,
                               std::shared_ptr<SwapChain> swapChain) override;

    void endOffscreenRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer) override;

    void endShadowRendering(uint32_t imageIndex,
                            const std::shared_ptr<CommandBuffer>& commandBuffer) override;

  private:
    std::shared_ptr<Framebuffer> m_swapchainFramebuffer;
    std::shared_ptr<Framebuffer> m_offscreenFramebuffer;

    std::shared_ptr<RenderPass> m_swapchainRenderPass;
    std::shared_ptr<RenderPass> m_offscreenRenderPass;

    static void endRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);
  };

} // namespace vke

#endif //VKE_LEGACYRENDERER_H
