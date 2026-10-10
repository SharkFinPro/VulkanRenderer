#ifndef VKE_RENDERTARGET_H
#define VKE_RENDERTARGET_H

#include "ImageResource.h"
#include "../pipelines/descriptorSets/DescriptorSet.h"
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  class RenderTarget {
  public:
    explicit RenderTarget(std::shared_ptr<LogicalDevice> logicalDevice,
                          vk::CommandPool commandPool);

    [[nodiscard]] ImageResource& getOffscreenResolveImageResource(uint32_t currentFrame);

    [[nodiscard]] ImageResource& getOffscreenRayTracingImageResource(uint32_t currentFrame);

    [[nodiscard]] ImageResource& getMousePickingColorImageResource(uint32_t currentFrame);

    [[nodiscard]] vk::DescriptorSetLayout getOffscreenImageDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSet getOffscreenImageDescriptorSet(uint32_t currentFrame) const;

    [[nodiscard]] vk::DescriptorSetLayout getOutlineMaskDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSet getOutlineMaskDescriptorSet(uint32_t currentFrame) const;

    void recreateImageResources(vk::Extent2D extent);

    void beginOffscreenRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                 uint32_t currentFrame) const;

    void endOffscreenRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                               uint32_t currentFrame) const;

    void beginMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    uint32_t currentFrame) const;

    void beginOutlineMaskRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   uint32_t currentFrame) const;

    // Leaves the mask readable by the fragment shader of the composite pass.
    void endOutlineMaskRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                 uint32_t currentFrame) const;

    void beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                  uint32_t currentFrame) const;

    // With drawOverlay the ray traced image is left as a color attachment for beginOverlayRendering, and
    // endOverlayRendering hands it over to the swapchain pass instead.
    void endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                uint32_t currentFrame,
                                bool drawOverlay) const;

    // Draws over the ray traced image, keeping what the copy put there.
    void beginOverlayRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                               uint32_t currentFrame) const;

    void endOverlayRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                             uint32_t currentFrame) const;

  protected:
    static constexpr vk::ClearValue s_clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    // The picking image is an integer format, which a float clear color can't be used with.
    static constexpr vk::ClearValue s_clearMousePicking = vk::ClearColorValue(std::array<uint32_t, 4>{ 0, 0, 0, 0 });
    static constexpr vk::ClearValue s_clearMask = vk::ClearColorValue(std::array<uint32_t, 4>{0, 0, 0, 0});
    static constexpr vk::ClearValue s_clearDepth = vk::ClearDepthStencilValue{
      .depth = 1.0f,
      .stencil = 0
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::CommandPool m_commandPool = nullptr;

    vk::raii::Sampler m_sampler = nullptr;

    // The mask holds ids, which no filter can blend
    vk::raii::Sampler m_maskSampler = nullptr;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    std::unique_ptr<DescriptorSet> m_offscreenImageDescriptorSet;

    std::unique_ptr<DescriptorSet> m_outlineMaskDescriptorSet;
    std::vector<vk::DescriptorImageInfo> m_outlineMaskImageInfos;

    vk::Extent2D m_extent{0, 0};

    std::vector<ImageResource> m_offscreenColorImageResources;
    std::vector<ImageResource> m_offscreenDepthImageResources;
    std::vector<ImageResource> m_offscreenResolveImageResources;

    std::vector<ImageResource> m_offscreenRayTracingImageResources;

    std::vector<ImageResource> m_mousePickingColorImageResources;
    std::vector<ImageResource> m_mousePickingDepthImageResources;

    std::vector<ImageResource> m_outlineMaskImageResources;

    void createSampler();

    void createDescriptorPool();

    void createOffscreenImageDescriptorSet();

    void createOutlineMaskDescriptorSet();

    void createOffscreenImageResources(vk::Extent2D extent);

    void createMousePickingImageResources(vk::Extent2D extent);

    void createOutlineMaskImageResources(vk::Extent2D extent);

    void transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          uint32_t currentFrame) const;

    void transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           uint32_t currentFrame,
                                           bool drawOverlay) const;

    void copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             uint32_t currentFrame) const;
  };

} // namespace vke

#endif //VKE_RENDERTARGET_H
