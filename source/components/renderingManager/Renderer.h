#ifndef VKE_RENDERER_H
#define VKE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class ImageResource;
  class LogicalDevice;

  class Renderer {
  public:
    explicit Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                      vk::CommandPool commandPool);

    [[nodiscard]] ImageResource& getOffscreenResolveImageResource(uint32_t currentFrame);

    [[nodiscard]] ImageResource& getOffscreenRayTracingImageResource(uint32_t currentFrame);

    [[nodiscard]] ImageResource& getMousePickingColorImageResource(uint32_t currentFrame);

    void recreateImageResources(vk::Extent2D extent);

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

    vk::raii::Sampler m_sampler = nullptr;

    vk::Extent2D m_extent{0, 0};

    std::vector<ImageResource> m_offscreenColorImageResources;
    std::vector<ImageResource> m_offscreenDepthImageResources;
    std::vector<ImageResource> m_offscreenResolveImageResources;

    std::vector<ImageResource> m_offscreenRayTracingImageResources;

    std::vector<ImageResource> m_mousePickingColorImageResources;
    std::vector<ImageResource> m_mousePickingDepthImageResources;

    void createSampler();

    void createOffscreenImageResources(vk::Extent2D extent);

    void createMousePickingImageResources(vk::Extent2D extent);

    void transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          uint32_t currentFrame) const;

    void transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           uint32_t currentFrame) const;

    void copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             uint32_t currentFrame) const;
  };

} // namespace vke

#endif //VKE_RENDERER_H
