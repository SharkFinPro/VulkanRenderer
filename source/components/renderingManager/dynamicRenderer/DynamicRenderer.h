#ifndef VKE_DYNAMICRENDERER_H
#define VKE_DYNAMICRENDERER_H

#include "../Renderer.h"

namespace vke {

  class RenderTarget;

  class DynamicRenderer final : public Renderer {
  public:
    explicit DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<SwapChain>& swapChain,
                             VkCommandPool commandPool);

    [[nodiscard]] std::shared_ptr<RenderPass> getRenderPass() const override;

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

    void endSwapchainRendering(uint32_t imageIndex,
                               std::shared_ptr<CommandBuffer> commandBuffer,
                               std::shared_ptr<SwapChain> swapChain) override;

    void endOffscreenRendering(uint32_t imageIndex,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

    void endShadowRendering(uint32_t imageIndex,
                            const std::shared_ptr<CommandBuffer>& commandBuffer) override;

  private:
    static void transitionSwapchainImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  VkImage image);

    static void transitionSwapchainImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   VkImage image);
  };

} // namespace vke

#endif //VKE_DYNAMICRENDERER_H
