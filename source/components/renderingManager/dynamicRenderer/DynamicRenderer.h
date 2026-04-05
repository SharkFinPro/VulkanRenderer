#ifndef VKE_DYNAMICRENDERER_H
#define VKE_DYNAMICRENDERER_H

#include "../Renderer.h"

namespace vke {

  class RenderTarget;

  class DynamicRenderer final : public Renderer {
  public:
    explicit DynamicRenderer(std::shared_ptr<LogicalDevice> logicalDevice,
                             const std::shared_ptr<SwapChain>& swapChain,
                             vk::CommandPool commandPool);

    void beginSwapchainRendering(uint32_t imageIndex,
                                 vk::Extent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer,
                                 std::shared_ptr<SwapChain> swapChain) override;

    void beginOffscreenRendering(uint32_t currentFrame,
                                 vk::Extent2D extent,
                                 std::shared_ptr<CommandBuffer> commandBuffer) override;

    void beginShadowRendering(uint32_t currentFrame,
                              vk::Extent2D extent,
                              const std::shared_ptr<CommandBuffer>& commandBuffer,
                              const std::shared_ptr<Light>& light) override;

    void beginMousePickingRendering(uint32_t currentFrame,
                                    vk::Extent2D extent,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    void endSwapchainRendering(uint32_t imageIndex,
                               std::shared_ptr<CommandBuffer> commandBuffer,
                               std::shared_ptr<SwapChain> swapChain) override;

    void endOffscreenRendering(std::shared_ptr<CommandBuffer> commandBuffer) override;

    void endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    void endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) override;

    [[nodiscard]] bool supportsRayTracing() const override;

    void beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                  uint32_t currentFrame) override;

    void endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                uint32_t currentFrame) override;

  private:
    static constexpr vk::ClearValue s_clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    static void transitionSwapchainImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                  vk::Image image);

    static void transitionSwapchainImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                   vk::Image image);

    void transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          uint32_t currentFrame) const;

    void transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           uint32_t currentFrame) const;

    void copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             uint32_t currentFrame) const;
  };

} // namespace vke

#endif //VKE_DYNAMICRENDERER_H
