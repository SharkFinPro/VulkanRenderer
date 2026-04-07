#ifndef VKE_RENDERER_H
#define VKE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class ImageResource;
  class Light;
  class LogicalDevice;
  class RenderTarget;
  class SwapChain;

  class Renderer {
  public:
    explicit Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                      const std::shared_ptr<SwapChain>& swapChain,
                      vk::CommandPool commandPool);

    [[nodiscard]] vk::DescriptorSet getOffscreenImageDescriptorSet(uint32_t imageIndex) const;

    [[nodiscard]] std::shared_ptr<RenderTarget> getMousePickingRenderTarget() const;

    void resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain);

    void resetOffscreenImageResources(vk::Extent2D offscreenViewportExtent);

    void resetMousePickingImageResources(vk::Extent2D mousePickingExtent);

    void resetRayTracingImageResources(vk::Extent2D extent);

    void beginSwapchainRendering(uint32_t imageIndex,
                                 vk::Extent2D extent,
                                 const std::shared_ptr<CommandBuffer>& commandBuffer,
                                 const std::shared_ptr<SwapChain>& swapChain) const;

    void beginOffscreenRendering(uint32_t currentFrame,
                                         vk::Extent2D extent,
                                         const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    static void beginShadowRendering(vk::Extent2D extent,
                                     const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     const std::shared_ptr<Light>& light);

    void beginMousePickingRendering(uint32_t currentFrame,
                                    vk::Extent2D extent,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    static void endSwapchainRendering(uint32_t imageIndex,
                                      const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const std::shared_ptr<SwapChain>& swapChain);

    static void endOffscreenRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);

    static void endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);

    static void endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);

    void beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                  uint32_t currentFrame) const;

    void endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                uint32_t currentFrame) const;

    [[nodiscard]] std::shared_ptr<ImageResource> getRayTracingImageResource(uint32_t currentFrame) const;

  protected:
    static constexpr vk::ClearValue s_clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::CommandPool m_commandPool = nullptr;

    std::shared_ptr<RenderTarget> m_offscreenRenderTarget;

    std::shared_ptr<RenderTarget> m_swapchainRenderTarget;

    std::shared_ptr<RenderTarget> m_mousePickingRenderTarget;

    std::vector<std::shared_ptr<ImageResource>> m_rayTracingImageResources;

    vk::raii::Sampler m_sampler = nullptr;

    void createSampler();

    void createSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain);

    void createOffscreenRenderTarget(vk::Extent2D extent);

    void createMousePickingRenderTarget(vk::Extent2D extent);

    void createRayTracingImageResource(vk::Extent2D extent);
    
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

#endif //VKE_RENDERER_H
