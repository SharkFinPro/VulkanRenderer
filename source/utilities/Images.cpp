#include "Images.h"
#include "Buffers.h"
#include "../components/commandBuffer/SingleUseCommandBuffer.h"
#include "../components/logicalDevice/LogicalDevice.h"
#include "../components/physicalDevice/PhysicalDevice.h"
#include <stdexcept>

namespace vke::Images {

  std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                 const ImageConfig& imageConfig)
  {
    const vk::ImageCreateInfo imageCreateInfo{
      .flags = imageConfig.flags,
      .imageType = imageConfig.imageType,
      .format = imageConfig.format,
      .extent = imageConfig.extent,
      .mipLevels = imageConfig.mipLevels,
      .arrayLayers = imageConfig.layerCount,
      .samples = imageConfig.numSamples,
      .tiling = imageConfig.tiling,
      .usage = imageConfig.usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined
    };

    vk::raii::Image image = logicalDevice->createImage(imageCreateInfo);

    const vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();

    const vk::MemoryAllocateInfo allocateInfo {
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = logicalDevice->getPhysicalDevice()->findMemoryType(memoryRequirements.memoryTypeBits,
                                                                            imageConfig.properties)
    };

    vk::raii::DeviceMemory imageMemory = nullptr;
    logicalDevice->allocateMemory(allocateInfo, imageMemory);

    image.bindMemory(*imageMemory, 0);

    return { std::move(image), std::move(imageMemory) };
  }

  struct TransitionInfo {
    vk::ImageAspectFlags aspectMask;
    vk::AccessFlags2 srcAccessMask;
    vk::AccessFlags2 dstAccessMask;
    vk::PipelineStageFlags2 sourceStage;
    vk::PipelineStageFlags2 destinationStage;
  };

  struct LayoutPair {
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;

    bool operator==(const LayoutPair& other) const
    {
      return oldLayout == other.oldLayout &&
             newLayout == other.newLayout;
    }
  };

  struct LayoutPairHash {
    std::size_t operator()(const LayoutPair& pair) const
    {
      return std::hash<int>()(static_cast<int>(pair.oldLayout)) ^
             (std::hash<int>()(static_cast<int>(pair.newLayout)) << 1);
    }
  };

  const std::unordered_map<LayoutPair, TransitionInfo, LayoutPairHash> transitionMap {
    {
      { vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal },
      {
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .sourceStage = vk::PipelineStageFlagBits2::eNone,
        .destinationStage = vk::PipelineStageFlagBits2::eTransfer
      }
    },
    {
      { vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal },
      {
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
        .sourceStage = vk::PipelineStageFlagBits2::eTransfer,
        .destinationStage = vk::PipelineStageFlagBits2::eFragmentShader
      }
    },
    {
      {vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal},
      {
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                         vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .sourceStage = vk::PipelineStageFlagBits2::eNone,
        .destinationStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits2::eLateFragmentTests
      }
    },
    {
      {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal},
      {
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
        .sourceStage = vk::PipelineStageFlagBits2::eNone,
        .destinationStage = vk::PipelineStageFlagBits2::eFragmentShader
      }
    },
    {
      {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal},
      {
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentRead |
                         vk::AccessFlagBits2::eColorAttachmentWrite,
        .sourceStage = vk::PipelineStageFlagBits2::eNone,
        .destinationStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput
      }
    },
    {
      { vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal },
      {
        .srcAccessMask    = vk::AccessFlagBits2::eShaderSampledRead,
        .dstAccessMask    = vk::AccessFlagBits2::eColorAttachmentRead |
                            vk::AccessFlagBits2::eColorAttachmentWrite,
        .sourceStage      = vk::PipelineStageFlagBits2::eFragmentShader,
        .destinationStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput
      }
    },
    {
      { vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal },
      {
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentRead |
                         vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
        .sourceStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .destinationStage = vk::PipelineStageFlagBits2::eFragmentShader
      }
    },
    {
      { vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral },
      {
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits2::eShaderRead |
                         vk::AccessFlagBits2::eShaderWrite,
        .sourceStage = vk::PipelineStageFlagBits2::eNone,
        .destinationStage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR
      }
    }
  };

  TransitionInfo getTransitionInfo(const vk::ImageLayout oldLayout,
                                   const vk::ImageLayout newLayout,
                                   const vk::Format format)
  {
    const LayoutPair key { oldLayout, newLayout };
    const auto it = transitionMap.find(key);

    if (it == transitionMap.end())
    {
      throw std::invalid_argument("unsupported layout transition!");
    }

    TransitionInfo transitionInfo = it->second;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
      transitionInfo.aspectMask = vk::ImageAspectFlagBits::eDepth;

      if (hasStencilComponent(format))
      {
        transitionInfo.aspectMask |= vk::ImageAspectFlagBits::eStencil;
      }
    }
    else
    {
      transitionInfo.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    return transitionInfo;
  }

  void recordImageLayoutTransition(const CommandBuffer& commandBuffer,
                                   vk::Image image,
                                   const vk::Format format,
                                   const vk::ImageLayout oldLayout,
                                   const vk::ImageLayout newLayout,
                                   const uint32_t mipLevels,
                                   const uint32_t layerCount)
  {
    const auto [aspectMask,
                srcAccessMask,
                dstAccessMask,
                sourceStage,
                destinationStage] = getTransitionInfo(oldLayout, newLayout, format);

    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = sourceStage,
      .srcAccessMask = srcAccessMask,
      .dstStageMask = destinationStage,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange = {
        .aspectMask = aspectMask,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = layerCount
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer.pipelineBarrier(dependencyInfo);
  }

  void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const vk::CommandPool commandPool,
                             vk::Image image,
                             const vk::Format format,
                             const vk::ImageLayout oldLayout,
                             const vk::ImageLayout newLayout,
                             const uint32_t mipLevels,
                             const uint32_t layerCount)
  {
    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, image, format, oldLayout, newLayout, mipLevels, layerCount] {
      recordImageLayoutTransition(commandBuffer, image, format, oldLayout, newLayout, mipLevels, layerCount);
    });
  }

  void recordCopyBufferToImage(const CommandBuffer& commandBuffer,
                               vk::Buffer buffer,
                               vk::Image image,
                               const uint32_t width,
                               const uint32_t height,
                               const uint32_t depth)
  {
    const vk::BufferImageCopy region {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .imageOffset = { 0, 0, 0 },
      .imageExtent = { width, height, depth }
    };

    commandBuffer.copyBufferToImage(
      buffer,
      image,
      vk::ImageLayout::eTransferDstOptimal,
      { region }
    );
  }

  void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const vk::CommandPool commandPool,
                         vk::Buffer buffer,
                         vk::Image image,
                         const uint32_t width,
                         const uint32_t height,
                         const uint32_t depth)
  {
    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, buffer, image, width, height, depth] {
      recordCopyBufferToImage(commandBuffer, buffer, image, width, height, depth);
    });
  }

  void copyImageToBuffer(const vk::Image image,
                         const vk::Offset3D offset,
                         const vk::Extent3D extent,
                         const CommandBuffer& commandBuffer,
                         const vk::Buffer stagingBuffer)
  {
    const vk::BufferImageCopy region {
      .bufferOffset      = 0,
      .bufferRowLength   = 0,
      .bufferImageHeight = 0,
      .imageSubresource  = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .imageOffset = offset,
      .imageExtent = extent
    };

    commandBuffer.copyImageToBuffer(
      image,
      vk::ImageLayout::eTransferSrcOptimal,
      stagingBuffer,
      { region }
    );
  }

  vk::raii::ImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                       const vk::Image image,
                                       const vk::Format format,
                                       const vk::ImageAspectFlags aspectFlags,
                                       const uint32_t mipLevels,
                                       const vk::ImageViewType viewType,
                                       const uint32_t layerCount)
  {
    const vk::ImageViewCreateInfo imageViewCreateInfo {
      .image = image,
      .viewType = viewType,
      .format = format,
      .components = {
        .r = vk::ComponentSwizzle::eIdentity,
        .g = vk::ComponentSwizzle::eIdentity,
        .b = vk::ComponentSwizzle::eIdentity,
        .a = vk::ComponentSwizzle::eIdentity
      },
      .subresourceRange = {
        .aspectMask = aspectFlags,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = layerCount
      }
    };

    return logicalDevice->createImageView(imageViewCreateInfo);
  }

  bool hasStencilComponent(const vk::Format format)
  {
    return format == vk::Format::eD32SfloatS8Uint ||
           format == vk::Format::eD24UnormS8Uint;
  }

} // namespace vke::Images