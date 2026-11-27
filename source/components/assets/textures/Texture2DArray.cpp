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

    void barrier(const SingleUseCommandBuffer& commandBuffer,
                 const vk::Image image,
                 const vk::ImageSubresourceRange& range,
                 const vk::ImageLayout oldLayout,
                 const vk::ImageLayout newLayout,
                 const vk::PipelineStageFlags2 srcStage,
                 const vk::AccessFlags2 srcAccess,
                 const vk::PipelineStageFlags2 dstStage,
                 const vk::AccessFlags2 dstAccess)
    {
      const vk::ImageMemoryBarrier2 imageBarrier {
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
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
    uint32_t width = 1;
    uint32_t height = 1;
    bool anySource = false;

    for (const Texture2DArrayLayer& layer : layers)
    {
      if (layer.source)
      {
        const vk::Extent2D extent = layer.source->getExtent();
        width = anySource ? std::max(width, extent.width) : extent.width;
        height = anySource ? std::max(height, extent.height) : extent.height;
        anySource = true;
      }
    }

    m_mipLevels = static_cast<uint32_t>(std::bit_width(std::max(width, height)));

    const auto formatProperties = logicalDevice->getPhysicalDevice()->getFormatProperties(s_format);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
      throw std::runtime_error("texture image format does not support linear blitting!");
    }

    auto [image, imageMemory] = Images::createImage(
      logicalDevice,
      {
        {},
        vk::Extent3D{ width, height, 1 },
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
    const uint32_t mipLevels = m_mipLevels;
    const uint32_t layerCount = m_layerCount;

    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&] {
      using Stage = vk::PipelineStageFlagBits2;
      using Access = vk::AccessFlagBits2;
      using Layout = vk::ImageLayout;

      barrier(commandBuffer, dst, colorRange(0, mipLevels, 0, layerCount),
              Layout::eUndefined, Layout::eTransferDstOptimal,
              Stage::eTopOfPipe, {}, Stage::eTransfer, Access::eTransferWrite);

      for (uint32_t i = 0; i < layerCount; i++)
      {
        const Texture2DArrayLayer& layer = layers[i];

        if (!layer.source)
        {
          const vk::ClearColorValue color { std::array { layer.fill.r, layer.fill.g, layer.fill.b, layer.fill.a } };

          commandBuffer.clearColorImage(dst, Layout::eTransferDstOptimal, color, colorRange(0, 1, i, 1));

          continue;
        }

        const vk::Image src = layer.source->getImage();
        const vk::Extent2D srcExtent = layer.source->getExtent();
        const auto srcRange = colorRange(0, 1, 0, 1);

        // Earlier frames on the queue may still be sampling the source
        barrier(commandBuffer, src, srcRange, Layout::eShaderReadOnlyOptimal, Layout::eTransferSrcOptimal,
                Stage::eAllCommands, Access::eShaderSampledRead | Access::eShaderRead,
                Stage::eTransfer, Access::eTransferRead);

        const vk::ImageBlit blit {
          .srcSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
          .srcOffsets = std::array {
            vk::Offset3D{ 0, 0, 0 },
            vk::Offset3D{ static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
          },
          .dstSubresource = { vk::ImageAspectFlagBits::eColor, 0, i, 1 },
          .dstOffsets = std::array {
            vk::Offset3D{ 0, 0, 0 },
            vk::Offset3D{ static_cast<int32_t>(width), static_cast<int32_t>(height), 1 }
          }
        };

        commandBuffer.blitImage(src, Layout::eTransferSrcOptimal, dst, Layout::eTransferDstOptimal, blit,
                                vk::Filter::eLinear);

        barrier(commandBuffer, src, srcRange, Layout::eTransferSrcOptimal, Layout::eShaderReadOnlyOptimal,
                Stage::eTransfer, Access::eTransferRead, Stage::eAllCommands, Access::eShaderSampledRead);
      }

      int32_t mipWidth = static_cast<int32_t>(width);
      int32_t mipHeight = static_cast<int32_t>(height);

      for (uint32_t level = 1; level < mipLevels; level++)
      {
        barrier(commandBuffer, dst, colorRange(level - 1, 1, 0, layerCount),
                Layout::eTransferDstOptimal, Layout::eTransferSrcOptimal,
                Stage::eTransfer, Access::eTransferWrite, Stage::eTransfer, Access::eTransferRead);

        const int32_t nextWidth = mipWidth > 1 ? mipWidth / 2 : 1;
        const int32_t nextHeight = mipHeight > 1 ? mipHeight / 2 : 1;

        const vk::ImageBlit blit {
          .srcSubresource = { vk::ImageAspectFlagBits::eColor, level - 1, 0, layerCount },
          .srcOffsets = std::array { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ mipWidth, mipHeight, 1 } },
          .dstSubresource = { vk::ImageAspectFlagBits::eColor, level, 0, layerCount },
          .dstOffsets = std::array { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ nextWidth, nextHeight, 1 } }
        };

        commandBuffer.blitImage(dst, Layout::eTransferSrcOptimal, dst, Layout::eTransferDstOptimal, blit,
                                vk::Filter::eLinear);

        barrier(commandBuffer, dst, colorRange(level - 1, 1, 0, layerCount),
                Layout::eTransferSrcOptimal, Layout::eShaderReadOnlyOptimal,
                Stage::eTransfer, Access::eTransferRead, Stage::eFragmentShader, Access::eShaderSampledRead);

        mipWidth = nextWidth;
        mipHeight = nextHeight;
      }

      barrier(commandBuffer, dst, colorRange(mipLevels - 1, 1, 0, layerCount),
              Layout::eTransferDstOptimal, Layout::eShaderReadOnlyOptimal,
              Stage::eTransfer, Access::eTransferWrite, Stage::eFragmentShader, Access::eShaderSampledRead);
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
