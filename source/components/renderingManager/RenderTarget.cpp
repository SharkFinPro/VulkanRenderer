#include "RenderTarget.h"
#include "ImageResource.h"

namespace vke {
  RenderTarget::RenderTarget(const ImageResourceConfig& imageResourceConfig,
                             const uint32_t numImages)
    : m_extent(imageResourceConfig.extent)
  {
    m_colorImageResources.reserve(numImages);
    m_depthImageResources.reserve(numImages);
    m_resolveImageResources.reserve(numImages);

    auto colorImageResourceConfig = imageResourceConfig;
    colorImageResourceConfig.imageResourceType = ImageResourceType::Color;

    auto depthImageResourceConfig = imageResourceConfig;
    depthImageResourceConfig.imageResourceType = ImageResourceType::Depth;

    auto resolveImageResourceConfig = imageResourceConfig;
    resolveImageResourceConfig.imageResourceType = ImageResourceType::Resolve;
    resolveImageResourceConfig.numSamples = vk::SampleCountFlagBits::e1;

    for (int i = 0; i < numImages; ++i)
    {
      if (imageResourceConfig.colorFormat != vk::Format::eUndefined)
      {
        m_colorImageResources.emplace_back(colorImageResourceConfig);
      }

      if (imageResourceConfig.depthFormat != vk::Format::eUndefined)
      {
        m_depthImageResources.emplace_back(depthImageResourceConfig);
      }

      if (imageResourceConfig.resolveFormat != vk::Format::eUndefined)
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

  vk::Extent2D RenderTarget::getExtent() const
  {
    return m_extent;
  }
} // vke