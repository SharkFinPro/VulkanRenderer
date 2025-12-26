#ifndef VULKANPROJECT_RENDERTARGET_H
#define VULKANPROJECT_RENDERTARGET_H

#include <vector>

namespace vke {

  class ImageResource;
  struct ImageResourceConfig;
  class LogicalDevice;

  class RenderTarget {
  public:
    explicit RenderTarget(const ImageResourceConfig& imageResourceConfig);

  private:
    std::vector<ImageResource> m_colorImageResources;
    std::vector<ImageResource> m_depthImageResources;
    std::vector<ImageResource> m_resolveImageResources;
  };
} // vke

#endif //VULKANPROJECT_RENDERTARGET_H