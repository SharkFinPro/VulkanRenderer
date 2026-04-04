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
  class RenderPass;
  class RenderTarget;
  class SwapChain;

  class Renderer {
  public:
    explicit Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                      const std::shared_ptr<SwapChain>& swapChain,
                      vk::raii::CommandPool commandPool);

    virtual ~Renderer() = default;

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getSwapchainRenderPass() const { return nullptr; };

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getOffscreenRenderPass() const { return nullptr; };

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getShadowRenderPass() const { return nullptr; };

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getShadowCubeRenderPass() const { return nullptr; };

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getMousePickingRenderPass() const { return nullptr; };

    [[nodiscard]] virtual vk::DescriptorSet getOffscreenImageDescriptorSet(uint32_t imageIndex);

    [[nodiscard]] std::shared_ptr<RenderTarget> getMousePickingRenderTarget() const;

    virtual void resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain);

    virtual void resetOffscreenImageResources(vk::Extent2D offscreenViewportExtent);

    virtual void resetMousePickingImageResources(vk::Extent2D mousePickingExtent);

    virtual void resetRayTracingImageResources(vk::Extent2D extent);

    virtual void beginSwapchainRendering(uint32_t imageIndex,
                                         vk::Extent2D extent,
                                         std::shared_ptr<CommandBuffer> commandBuffer,
                                         std::shared_ptr<SwapChain> swapChain) = 0;

    virtual void beginOffscreenRendering(uint32_t currentFrame,
                                         vk::Extent2D extent,
                                         std::shared_ptr<CommandBuffer> commandBuffer) = 0;

    virtual void beginShadowRendering(uint32_t currentFrame,
                                      vk::Extent2D extent,
                                      const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const std::shared_ptr<Light>& light) = 0;

    virtual void beginMousePickingRendering(uint32_t currentFrame,
                                            vk::Extent2D extent,
                                            const std::shared_ptr<CommandBuffer>& commandBuffer) = 0;

    virtual void endSwapchainRendering(uint32_t imageIndex,
                                       std::shared_ptr<CommandBuffer> commandBuffer,
                                       std::shared_ptr<SwapChain> swapChain) = 0;

    virtual void endOffscreenRendering(std::shared_ptr<CommandBuffer> commandBuffer) = 0;

    virtual void endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) = 0;

    virtual void endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer) = 0;

    [[nodiscard]] virtual uint32_t registerShadowMapRenderTarget(std::shared_ptr<RenderTarget> renderTarget,
                                                                 bool isCubeMap);

    [[nodiscard]] virtual bool supportsRayTracing() const { return false; }

    virtual void beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          uint32_t currentFrame) {}

    virtual void endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                        uint32_t currentFrame) {}

    [[nodiscard]] std::shared_ptr<ImageResource> getRayTracingImageResource(uint32_t currentFrame) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::CommandPool m_commandPool = nullptr;

    std::shared_ptr<RenderTarget> m_offscreenRenderTarget;

    std::shared_ptr<RenderTarget> m_swapchainRenderTarget;

    std::shared_ptr<RenderTarget> m_mousePickingRenderTarget;

    std::vector<std::shared_ptr<ImageResource>> m_rayTracingImageResources;

    vk::raii::Sampler m_sampler = nullptr;

    uint32_t m_currentShadowMapRenderTargetID = 0;

    void createSampler();

    void createSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain);

    void createOffscreenRenderTarget(vk::Extent2D extent);

    void createMousePickingRenderTarget(vk::Extent2D extent);

    void createRayTracingImageResource(vk::Extent2D extent);
  };

} // namespace vke

#endif //VKE_RENDERER_H
