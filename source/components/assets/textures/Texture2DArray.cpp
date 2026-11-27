#include "Texture2DArray.h"
#include "Texture2D.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Images.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace vke {

  namespace {

    constexpr vk::Format s_format = vk::Format::eR8G8B8A8Unorm;

    using Layout = vk::ImageLayout;

    struct Scope {
      vk::PipelineStageFlags2 stage;
      vk::AccessFlags2 access;
    };

    constexpr Scope s_transferWrite { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite };
    constexpr Scope s_transferRead { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead };
    // Every stage that may sample the array: fragment and, with ray tracing, the closest hit shader
    constexpr Scope s_sampled { vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eShaderSampledRead };

    void barrier(const SingleUseCommandBuffer& commandBuffer,
                 const vk::Image image,
                 const vk::ImageSubresourceRange& range,
                 const Layout oldLayout,
                 const Layout newLayout,
                 const Scope& src,
                 const Scope& dst)
    {
      const vk::ImageMemoryBarrier2 imageBarrier {
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dst.stage,
        .dstAccessMask = dst.access,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = range
      };

      const vk::DependencyInfo dependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
      };

      commandBuffer.pipelineBarrier(dependencyInfo);
    }

    vk::ImageSubresourceRange colorRange(const uint32_t baseMip,
                                         const uint32_t mipCount,
                                         const uint32_t baseLayer,
                                         const uint32_t layerCount)
    {
      return {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = baseMip,
        .levelCount = mipCount,
        .baseArrayLayer = baseLayer,
        .layerCount = layerCount
      };
    }

    // The largest width and height among the sources, 1x1 with none
    vk::Extent2D arrayExtent(const std::span<const Texture2DArrayLayer> layers)
    {
      vk::Extent2D extent { 1, 1 };

      for (const Texture2DArrayLayer& layer : layers)
      {
        if (layer.source)
        {
          const vk::Extent2D sourceExtent = layer.source->getExtent();
          extent.width = std::max(extent.width, sourceExtent.width);
          extent.height = std::max(extent.height, sourceExtent.height);
        }
      }

      return extent;
    }

    void checkBlitSupport(const std::shared_ptr<LogicalDevice>& logicalDevice)
    {
      const auto features = logicalDevice->getPhysicalDevice()->getFormatProperties(s_format).optimalTilingFeatures;
      constexpr auto required = vk::FormatFeatureFlagBits::eSampledImageFilterLinear |
                                vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;

      if ((features & required) != required)
      {
        throw std::runtime_error("texture image format does not support linear blitting!");
      }
    }

    vk::ImageBlit makeBlit(const vk::ImageSubresourceLayers& src,
                           const vk::Extent2D srcExtent,
                           const vk::ImageSubresourceLayers& dst,
                           const vk::Extent2D dstExtent)
    {
      return {
        .srcSubresource = src,
        .srcOffsets = std::array {
          vk::Offset3D{ 0, 0, 0 },
          vk::Offset3D{ static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
        },
        .dstSubresource = dst,
        .dstOffsets = std::array {
          vk::Offset3D{ 0, 0, 0 },
          vk::Offset3D{ static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
        }
      };
    }

    // Copies a source's mip 0 into a layer, scaling it to the array's extent
    void blitSourceToLayer(const SingleUseCommandBuffer& commandBuffer,
                           const vk::Image dst,
                           const uint32_t layer,
                           const Texture2D& source,
                           const vk::Extent2D extent)
    {
      const vk::Image src = source.getImage();
      const auto srcRange = colorRange(0, 1, 0, 1);

      // Earlier frames on the queue may still be sampling the source
      barrier(commandBuffer, src, srcRange, Layout::eShaderReadOnlyOptimal, Layout::eTransferSrcOptimal,
              { vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eShaderRead },
              s_transferRead);

      const vk::ImageBlit blit = makeBlit({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, source.getExtent(),
                                          { vk::ImageAspectFlagBits::eColor, 0, layer, 1 }, extent);

      commandBuffer.blitImage(src, Layout::eTransferSrcOptimal, dst, Layout::eTransferDstOptimal, blit,
                              vk::Filter::eLinear);

      barrier(commandBuffer, src, srcRange, Layout::eTransferSrcOptimal, Layout::eShaderReadOnlyOptimal,
              s_transferRead, s_sampled);
    }

    void fillLayers(const SingleUseCommandBuffer& commandBuffer,
                    const vk::Image dst,
                    const std::span<const Texture2DArrayLayer> layers,
                    const vk::Extent2D extent)
    {
      for (uint32_t i = 0; i < layers.size(); i++)
      {
        const Texture2DArrayLayer& layer = layers[i];

        if (layer.source)
        {
          blitSourceToLayer(commandBuffer, dst, i, *layer.source, extent);
        }
        else
        {
          const vk::ClearColorValue color { std::array { layer.fill.r, layer.fill.g, layer.fill.b, layer.fill.a } };

          commandBuffer.clearColorImage(dst, Layout::eTransferDstOptimal, color, colorRange(0, 1, i, 1));
        }
      }
    }

    // Every mip of every layer ends in ShaderReadOnlyOptimal
    void generateMips(const SingleUseCommandBuffer& commandBuffer,
                      const vk::Image dst,
                      const vk::Extent2D extent,
                      const uint32_t mipLevels,
                      const uint32_t layerCount)
    {
      vk::Extent2D mip = extent;

      for (uint32_t level = 1; level < mipLevels; level++)
      {
        const auto previous = colorRange(level - 1, 1, 0, layerCount);
        const vk::Extent2D next { std::max(mip.width / 2, 1u), std::max(mip.height / 2, 1u) };

        barrier(commandBuffer, dst, previous, Layout::eTransferDstOptimal, Layout::eTransferSrcOptimal,
                s_transferWrite, s_transferRead);

        const vk::ImageBlit blit = makeBlit({ vk::ImageAspectFlagBits::eColor, level - 1, 0, layerCount }, mip,
                                            { vk::ImageAspectFlagBits::eColor, level, 0, layerCount }, next);

        commandBuffer.blitImage(dst, Layout::eTransferSrcOptimal, dst, Layout::eTransferDstOptimal, blit,
                                vk::Filter::eLinear);

        barrier(commandBuffer, dst, previous, Layout::eTransferSrcOptimal, Layout::eShaderReadOnlyOptimal,
                s_transferRead, s_sampled);

        mip = next;
      }

      barrier(commandBuffer, dst, colorRange(mipLevels - 1, 1, 0, layerCount), Layout::eTransferDstOptimal,
              Layout::eShaderReadOnlyOptimal, s_transferWrite, s_sampled);
    }

  } // namespace

  Texture2DArray::Texture2DArray(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const vk::CommandPool commandPool,
                                 const std::span<const Texture2DArrayLayer> layers,
                                 const vk::SamplerAddressMode samplerAddressMode)
    : Texture(logicalDevice, samplerAddressMode)
  {
    if (layers.empty())
    {
      throw std::invalid_argument("A texture array needs at least one layer.");
    }

    m_layerCount = static_cast<uint32_t>(layers.size());

    createTextureImage(logicalDevice, commandPool, layers);

    createImageView(logicalDevice);
  }

  uint32_t Texture2DArray::getLayerCount() const
  {
    return m_layerCount;
  }

  void Texture2DArray::createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                          const vk::CommandPool commandPool,
                                          const std::span<const Texture2DArrayLayer> layers)
  {
    checkBlitSupport(logicalDevice);

    const vk::Extent2D extent = arrayExtent(layers);

    m_mipLevels = static_cast<uint32_t>(std::bit_width(std::max(extent.width, extent.height)));

    auto [image, imageMemory] = Images::createImage(
      logicalDevice,
      {
        {},
        vk::Extent3D{ extent.width, extent.height, 1 },
        m_mipLevels,
        vk::SampleCountFlagBits::e1,
        s_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageType::e2D,
        m_layerCount,
        vk::MemoryPropertyFlagBits::eDeviceLocal
      }
    );

    m_textureImage = std::move(image);
    m_textureImageMemory = std::move(imageMemory);

    const vk::Image dst = *m_textureImage;

    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&] {
      barrier(commandBuffer, dst, colorRange(0, m_mipLevels, 0, m_layerCount),
              Layout::eUndefined, Layout::eTransferDstOptimal,
              { vk::PipelineStageFlagBits2::eTopOfPipe, {} }, s_transferWrite);

      fillLayers(commandBuffer, dst, layers, extent);

      generateMips(commandBuffer, dst, extent, m_mipLevels, m_layerCount);
    });
  }

  void Texture2DArray::createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    m_textureImageView = Images::createImageView(
      logicalDevice,
      m_textureImage,
      s_format,
      vk::ImageAspectFlagBits::eColor,
      m_mipLevels,
      vk::ImageViewType::e2DArray,
      m_layerCount
    );

    m_imageInfo.imageView = *m_textureImageView;
  }

} // namespace vke
