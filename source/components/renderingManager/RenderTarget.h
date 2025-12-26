#ifndef VULKANPROJECT_RENDERTARGET_H
#define VULKANPROJECT_RENDERTARGET_H

#include <cstdint>
#include <vector>

namespace vke {

  class ImageResource;
  struct ImageResourceConfig;
  class LogicalDevice;

  constexpr uint32_t NUM_IMAGES = 3;

  class RenderTarget {
  public:
    explicit RenderTarget(const ImageResourceConfig& imageResourceConfig);

    [[nodiscard]] ImageResource& getColorImageResource(uint32_t imageIndex);

    [[nodiscard]] ImageResource& getDepthImageResource(uint32_t imageIndex);

    [[nodiscard]] ImageResource& getResolveImageResource(uint32_t imageIndex);

    [[nodiscard]] uint32_t hasColorImageResource() const;

    [[nodiscard]] uint32_t hasDepthImageResource() const;

    [[nodiscard]] uint32_t hasResolveImageResource() const;

  private:
    std::vector<ImageResource> m_colorImageResources;
    std::vector<ImageResource> m_depthImageResources;
    std::vector<ImageResource> m_resolveImageResources;
  };
} // vke

#endif //VULKANPROJECT_RENDERTARGET_H