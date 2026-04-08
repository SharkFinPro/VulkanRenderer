#include "RenderTarget.h"
#include "ImageResource.h"

namespace vke {
  RenderTarget::RenderTarget(const ImageResourceConfig& imageResourceConfig,
                             const uint32_t numImages,
                             const bool createRayTracingResources)
    : m_extent(imageResourceConfig.extent)
  {
    if (createRayTracingResources)
    {
      createRayTracingImageResources(imageResourceConfig, numImages);
    }

    createRasterizationImageResources(imageResourceConfig, numImages);
  }

  ImageResource& RenderTarget::getColorImageResource(const uint32_t imageIndex)
  {
    return m_colorImageResources.at(imageIndex);
  }

  ImageResource& RenderTarget::getDepthImageResource(const uint32_t imageIndex)
  {
    return m_depthImageResources.at(imageIndex);
  }

  ImageResource& RenderTarget::getResolveImageResource(const uint32_t imageIndex)
  {
    return m_resolveImageResources.at(imageIndex);
  }

  ImageResource& RenderTarget::getRayTracingImageResource(const uint32_t imageIndex)
  {
    return m_rayTracingImageResources.at(imageIndex);
  }

  vk::Extent2D RenderTarget::getExtent() const
  {
    return m_extent;
  }

  void RenderTarget::createRayTracingImageResources(ImageResourceConfig imageResourceConfig,
                                                    const uint32_t numImages)
  {
    imageResourceConfig.imageResourceType = ImageResourceType::RayTracingOutput;
    imageResourceConfig.colorFormat = vk::Format::eR8G8B8A8Uint;
    imageResourceConfig.numSamples = vk::SampleCountFlagBits::e1;

    m_rayTracingImageResources.reserve(numImages);

    for (uint32_t i = 0; i < numImages; ++i)
    {
      m_rayTracingImageResources.emplace_back(imageResourceConfig);
    }
  }

  void RenderTarget::createRasterizationImageResources(const ImageResourceConfig& imageResourceConfig,
                                                       const uint32_t numImages)
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

    for (uint32_t i = 0; i < numImages; ++i)
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
} // vke