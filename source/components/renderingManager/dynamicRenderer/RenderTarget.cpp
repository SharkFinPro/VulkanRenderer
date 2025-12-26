#include "RenderTarget.h"
#include "ImageResource.h"

constexpr uint32_t NUM_IMAGES = 3;

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
    resolveImageResourceConfig.numSamples = VK_SAMPLE_COUNT_1_BIT;

    for (int i = 0; i < NUM_IMAGES; ++i)
    {
      m_colorImageResources.emplace_back(colorImageResourceConfig);

      m_depthImageResources.emplace_back(depthImageResourceConfig);

      if (imageResourceConfig.resolveFormat != VK_FORMAT_UNDEFINED)
      {
        m_resolveImageResources.emplace_back(resolveImageResourceConfig);
      }
    }
  }

  ImageResource& RenderTarget::getColorImageResource(const uint32_t imageIndex)
  {
    return m_colorImageResources[imageIndex];
  }

  ImageResource& RenderTarget::getDepthImageResource(const uint32_t imageIndex)
  {
    return m_depthImageResources[imageIndex];
  }

  ImageResource& RenderTarget::getResolveImageResource(const uint32_t imageIndex)
  {
    return m_resolveImageResources[imageIndex];
  }
} // vke