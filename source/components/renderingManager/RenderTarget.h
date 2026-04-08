#ifndef VULKANPROJECT_RENDERTARGET_H
#define VULKANPROJECT_RENDERTARGET_H

#include <vulkan/vulkan_raii.hpp>
#include <vector>

namespace vke {

  class ImageResource;
  struct ImageResourceConfig;

  class RenderTarget {
  public:
    RenderTarget(const ImageResourceConfig& imageResourceConfig,
                 uint32_t numImages);

    [[nodiscard]] ImageResource& getColorImageResource(uint32_t imageIndex);

    [[nodiscard]] ImageResource& getDepthImageResource(uint32_t imageIndex);

    [[nodiscard]] ImageResource& getResolveImageResource(uint32_t imageIndex);

    [[nodiscard]] ImageResource& getRayTracingImageResource(uint32_t imageIndex);

    [[nodiscard]] vk::Extent2D getExtent() const;

  private:
    std::vector<ImageResource> m_colorImageResources;
    std::vector<ImageResource> m_depthImageResources;
    std::vector<ImageResource> m_resolveImageResources;

    std::vector<ImageResource> m_rayTracingImageResources;

    vk::Extent2D m_extent;

    void createRayTracingImageResources(const ImageResourceConfig& imageResourceConfig,
                                        uint32_t numImages);

    void createRasterizationImageResources(const ImageResourceConfig& imageResourceConfig,
                                          uint32_t numImages);
  };
} // vke

#endif //VULKANPROJECT_RENDERTARGET_H