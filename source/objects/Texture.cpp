#include "Texture.h"
#include <stdexcept>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../utilities/Buffers.h"
#include "../utilities/Images.h"

Texture::Texture(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
                 const VkCommandPool& commandPool, const char* path)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{
  createTextureImage(commandPool, path);

  createTextureSampler();

  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = textureImageView;
  imageInfo.sampler = textureSampler;
}

Texture::~Texture()
{
  vkDestroySampler(logicalDevice->getDevice(), textureSampler, nullptr);
  vkDestroyImageView(logicalDevice->getDevice(), textureImageView, nullptr);

  vkDestroyImage(logicalDevice->getDevice(), textureImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), textureImageMemory, nullptr);
}

VkDescriptorPoolSize Texture::getDescriptorPoolSize(const uint32_t MAX_FRAMES_IN_FLIGHT)
{
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

  return poolSize;
}

VkWriteDescriptorSet Texture::getDescriptorSet(const uint32_t binding, const VkDescriptorSet& dstSet) const
{
  const VkWriteDescriptorSet descriptorSet {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = dstSet,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &imageInfo
  };

  return descriptorSet;
}

void Texture::createTextureImage(const VkCommandPool& commandPool, const char* path)
{
  int texWidth, texHeight, texChannels;

  stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  if (!pixels)
  {
    throw std::runtime_error("failed to load texture image!");
  }

  mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

  const VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), imageSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  stbi_image_free(pixels);

  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), texWidth, texHeight, mipLevels,
                      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
  Images::copyBufferToImage(logicalDevice, commandPool, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                            static_cast<uint32_t>(texHeight));
  // Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

  vkDestroyBuffer(logicalDevice->getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), stagingBufferMemory, nullptr);

  generateMipmaps(commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);

  textureImageView = Images::createImageView(logicalDevice->getDevice(), textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void Texture::generateMipmaps(const VkCommandPool& commandPool, const VkImage image, VkFormat imageFormat,
                              const int32_t texWidth, const int32_t texHeight, const uint32_t mipLevels) const
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice->getPhysicalDevice(), imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  const VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice->getDevice(), commandPool);

  VkImageMemoryBarrier barrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    const VkImageBlit blit {
      .srcSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = i - 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .srcOffsets = {
        { 0, 0, 0 },
        { mipWidth, mipHeight, 1 }
      },
      .dstSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = i,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .dstOffsets = {
        { 0, 0, 0 },
        { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }
      }
    };

    vkCmdBlitImage(commandBuffer,
                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    if (mipWidth > 1)
    {
      mipWidth /= 2;
    }

    if (mipHeight > 1)
    {
      mipHeight /= 2;
    }
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  Buffers::endSingleTimeCommands(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);
}

void Texture::createTextureSampler()
{
  VkSamplerCreateInfo samplerCreateInfo{};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  samplerCreateInfo.anisotropyEnable = VK_TRUE;

  VkPhysicalDeviceProperties deviceProperties{};
  vkGetPhysicalDeviceProperties(physicalDevice->getPhysicalDevice(), &deviceProperties);

  samplerCreateInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

  samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

  samplerCreateInfo.compareEnable = VK_FALSE;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;

  if (vkCreateSampler(logicalDevice->getDevice(), &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture sampler!");
  }
}
