#include "Images.h"
#include "Buffers.h"
#include "../components/commandBuffer/SingleUseCommandBuffer.h"
#include "../components/logicalDevice/LogicalDevice.h"
#include "../components/physicalDevice/PhysicalDevice.h"
#include <stdexcept>

namespace vke::Images {

  void createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const ImageConfig& imageConfig,
                   VkImage& image,
                   VkDeviceMemory& imageMemory)
  {
    const VkImageCreateInfo imageCreateInfo {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .flags = imageConfig.flags,
      .imageType = imageConfig.imageType,
      .format = imageConfig.format,
      .extent = imageConfig.extent,
      .mipLevels = imageConfig.mipLevels,
      .arrayLayers = imageConfig.layerCount,
      .samples = imageConfig.numSamples,
      .tiling = imageConfig.tiling,
      .usage = imageConfig.usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    image = logicalDevice->createImage(imageCreateInfo);

    const VkMemoryRequirements memoryRequirements = logicalDevice->getImageMemoryRequirements(image);

    const VkMemoryAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = logicalDevice->getPhysicalDevice()->findMemoryType(memoryRequirements.memoryTypeBits,
                                                                            imageConfig.properties)
    };

    logicalDevice->allocateMemory(allocateInfo, imageMemory);

    logicalDevice->bindImageMemory(image, imageMemory);
  }

  struct TransitionInfo {
    VkImageAspectFlags aspectMask = 0;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
  };

  struct LayoutPair {
    VkImageLayout oldLayout;
    VkImageLayout newLayout;

    bool operator==(const LayoutPair& other) const
    {
      return oldLayout == other.oldLayout &&
             newLayout == other.newLayout;
    }
  };

  struct LayoutPairHash {
    std::size_t operator()(const LayoutPair& pair) const
    {
      return std::hash<int>()(pair.oldLayout) ^ (std::hash<int>()(pair.newLayout) << 1);
    }
  };

  const std::unordered_map<LayoutPair, TransitionInfo, LayoutPairHash> transitionMap {
    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      },
      {
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
      {
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      },
      {
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
      {
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      },
      {
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      },
      {
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
      }
    },

    {
      {
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
      {
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      }
    }
  };

  TransitionInfo getTransitionInfo(const VkImageLayout oldLayout,
                                   const VkImageLayout newLayout,
                                   const VkFormat format)
  {
    const LayoutPair key {
      oldLayout,
      newLayout
    };
    const auto it = transitionMap.find(key);

    if (it == transitionMap.end())
    {
      throw std::invalid_argument("unsupported layout transition!");
    }

    TransitionInfo transitionInfo = it->second;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      transitionInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if (hasStencilComponent(format))
      {
        transitionInfo.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
    }
    else
    {
      transitionInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return transitionInfo;
  }

  void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             VkCommandPool commandPool,
                             VkImage image,
                             const VkFormat format,
                             const VkImageLayout oldLayout,
                             const VkImageLayout newLayout,
                             const uint32_t mipLevels,
                             const uint32_t layerCount)
  {
    const auto [aspectMask,
                srcAccessMask,
                dstAccessMask,
                sourceStage,
                destinationStage] = getTransitionInfo(oldLayout, newLayout, format);

    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([commandBuffer, image, aspectMask, srcAccessMask, dstAccessMask, sourceStage,
                                          destinationStage, oldLayout, newLayout, mipLevels, layerCount] {
      const VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
          .aspectMask = aspectMask,
          .baseMipLevel = 0,
          .levelCount = mipLevels,
          .baseArrayLayer = 0,
          .layerCount = layerCount
        }
      };

      commandBuffer.pipelineBarrier(
        sourceStage,
        destinationStage,
        0,
        {},
        {},
        { imageMemoryBarrier }
      );
    });
  }

  void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         VkCommandPool commandPool,
                         VkBuffer buffer,
                         VkImage image,
                         const uint32_t width,
                         const uint32_t height,
                         const uint32_t depth)
  {
    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([commandBuffer, buffer, image, width, height, depth] {
      const VkBufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, depth}
      };

      commandBuffer.copyBufferToImage(
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        { region }
      );
    });
  }

  void copyImageToBuffer(const VkImage& image,
                         const VkOffset3D offset,
                         const VkExtent3D extent,
                         const SingleUseCommandBuffer& commandBuffer,
                         VkBuffer stagingBuffer)
  {
    const VkBufferImageCopy region{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .imageOffset = offset,
      .imageExtent = extent
    };

    commandBuffer.copyImageToBuffer(
      image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      stagingBuffer,
      { region }
    );
  }

  VkImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice,
                              VkImage image,
                              const VkFormat format,
                              const VkImageAspectFlags aspectFlags,
                              const uint32_t mipLevels,
                              const VkImageViewType viewType,
                              const uint32_t layerCount)
  {
    const VkImageViewCreateInfo imageViewCreateInfo {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = viewType,
      .format = format,

      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
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

  bool hasStencilComponent(const VkFormat format)
  {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

} // namespace vke::Images
