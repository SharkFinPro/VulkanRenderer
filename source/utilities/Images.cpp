#include "Images.h"
#include "Buffers.h"
#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"
#include <stdexcept>

namespace Images {
  void createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const std::shared_ptr<PhysicalDevice>& physicalDevice, const VkImageCreateFlags flags,
                   const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mipLevels,
                   const VkSampleCountFlagBits numSamples, const VkFormat format, const VkImageTiling tiling,
                   const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, VkImage& image,
                   VkDeviceMemory& imageMemory, const VkImageType imageType, const uint32_t layerCount)
  {
    const VkImageCreateInfo imageCreateInfo {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .flags = flags,
      .imageType = imageType,
      .format = format,
      .extent = {
        .width = width,
        .height = height,
        .depth = depth
      },
      .mipLevels = mipLevels,
      .arrayLayers = layerCount,
      .samples = numSamples,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (vkCreateImage(logicalDevice->getDevice(), &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(logicalDevice->getDevice(), image, &memoryRequirements);

    const VkMemoryAllocateInfo allocateInfo {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = physicalDevice->findMemoryType(memoryRequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(logicalDevice->getDevice(), &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(logicalDevice->getDevice(), image, imageMemory, 0);
  }

  void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             const VkImage image, const VkFormat format, const VkImageLayout oldLayout,
                             const VkImageLayout newLayout, const uint32_t mipLevels, const uint32_t layerCount)
  {
    const VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice, commandPool);

    VkImageMemoryBarrier barrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = layerCount
      }
    };

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if (hasStencilComponent(format))
      {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
    }
    else
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    Buffers::endSingleTimeCommands(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);
  }

  void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                         const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height,
                         const uint32_t depth)
  {
    const VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice, commandPool);

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

    vkCmdCopyBufferToImage(
      commandBuffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region
    );

    Buffers::endSingleTimeCommands(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);
  }

  void copyImageToBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkImage& image, VkOffset3D offset,
                         VkExtent3D extent, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer)
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

    vkCmdCopyImageToBuffer(
      commandBuffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      stagingBuffer,
      1,
      &region
    );
  }

  VkImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkImage image,
                              const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t mipLevels,
                              const VkImageViewType viewType, const uint32_t layerCount)
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

    VkImageView imageView;
    if (vkCreateImageView(logicalDevice->getDevice(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create image views!");
    }

    return imageView;
  }

  bool hasStencilComponent(const VkFormat format)
  {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  }
}
