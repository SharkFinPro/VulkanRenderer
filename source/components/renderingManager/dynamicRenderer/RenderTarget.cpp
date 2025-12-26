#include "RenderTarget.h"
#include "ImageResource.h"

constexpr uint32_t NUM_IMAGES = 2;

namespace vke {
  RenderTarget::RenderTarget(const ImageResourceConfig& imageResourceConfig)
  {
    m_colorImageResources.reserve(NUM_IMAGES);
    m_depthImageResources.reserve(NUM_IMAGES);
    m_resolveImageResources.reserve(NUM_IMAGES);

    auto colorImageResourceConfig = imageResourceConfig;
    colorImageResourceConfig.imageResourceType = ImageResourceType::Color;

    auto depthImageResourceConfig = imageResourceConfig;
    depthImageResourceConfig.imageResourceType = ImageResourceType::Depth;

    auto resolveImageResourceConfig = imageResourceConfig;
    resolveImageResourceConfig.imageResourceType = ImageResourceType::Resolve;

    for (int i = 0; i < NUM_IMAGES; ++i)
    {
      m_colorImageResources.emplace_back(colorImageResourceConfig);

      m_depthImageResources.emplace_back(depthImageResourceConfig);

      m_resolveImageResources.emplace_back(resolveImageResourceConfig);
    }
  }
} // vke