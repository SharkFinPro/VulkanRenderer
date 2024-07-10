#include "Images.h"
#include <stdexcept>

#include "Buffers.h"

void Images::createImage(VkDevice& device, VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height,
                         uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                         VkDeviceMemory& imageMemory)
{
  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.extent.width = width;
  imageCreateInfo.extent.height = height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = mipLevels;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.format = format;
  imageCreateInfo.tiling = tiling;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.usage = usage;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.samples = numSamples;
  imageCreateInfo.flags = 0;

  if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(device, image, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = memoryRequirements.size;
  allocateInfo.memoryTypeIndex = Buffers::findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
}

void Images::transitionImageLayout(VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, VkImage image,
                                   VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
  VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(device, commandPool);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

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

  Buffers::endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

void Images::copyBufferToImage(VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue, VkBuffer buffer,
                               VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(device, commandPool);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
    width,
    height,
    1
  };

  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  Buffers::endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

VkImageView Images::createImageView(VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.image = image;

  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format = format;

  imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;

  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image views!");
  }

  return imageView;
}

bool Images::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}