#ifndef VKE_RENDERER_H
#define VKE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class RenderTarget;

  class Renderer {
  public:
    explicit Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                      vk::CommandPool commandPool);

    [[nodiscard]] std::shared_ptr<RenderTarget> getOffscreenRenderTarget() const;

    [[nodiscard]] std::shared_ptr<RenderTarget> getMousePickingRenderTarget() const;

    void recreateRenderTargets(vk::Extent2D extent);

    void beginOffscreenRendering(uint32_t currentFrame,
                                 const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void beginMousePickingRendering(uint32_t currentFrame,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                  uint32_t currentFrame) const;

    void endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                uint32_t currentFrame) const;

  protected:
    static constexpr vk::ClearValue s_clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::CommandPool m_commandPool = nullptr;

    std::shared_ptr<RenderTarget> m_offscreenRenderTarget;

    std::shared_ptr<RenderTarget> m_mousePickingRenderTarget;

    vk::raii::Sampler m_sampler = nullptr;

    void createSampler();

    void createOffscreenRenderTarget(vk::Extent2D extent);

    void createMousePickingRenderTarget(vk::Extent2D extent);

    void transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          uint32_t currentFrame) const;

    void transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           uint32_t currentFrame) const;

    void copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             uint32_t currentFrame) const;
  };

} // namespace vke

#endif //VKE_RENDERER_H
