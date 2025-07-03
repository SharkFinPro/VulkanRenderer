#include "CubeMapTexture.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../utilities/Buffers.h"
#include "../../utilities/Images.h"
#include <stb_image.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

CubeMapTexture::CubeMapTexture(std::shared_ptr<LogicalDevice> logicalDevice,
                               std::shared_ptr<PhysicalDevice> physicalDevice,
                               const VkCommandPool& commandPool,
                               const std::array<std::string, 6>& paths)
  : logicalDevice(std::move(logicalDevice)), physicalDevice(std::move(physicalDevice))
{
  createTextureImage(commandPool, paths);

  createTextureSampler();

  imageInfo = {
    .sampler = textureSampler,
    .imageView = textureImageView,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };
}

CubeMapTexture::~CubeMapTexture()
{
  logicalDevice->destroySampler(textureSampler);
  logicalDevice->destroyImageView(textureImageView);

  logicalDevice->destroyImage(textureImage);
  logicalDevice->freeMemory(textureImageMemory);
}

VkDescriptorPoolSize CubeMapTexture::getDescriptorPoolSize(const uint32_t MAX_FRAMES_IN_FLIGHT)
{
  return {
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = MAX_FRAMES_IN_FLIGHT,
  };
}

VkWriteDescriptorSet CubeMapTexture::getDescriptorSet(const uint32_t binding, const VkDescriptorSet& dstSet) const
{
  return {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = dstSet,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &imageInfo
  };
}

void CubeMapTexture::createTextureSampler()
{
  const VkPhysicalDeviceProperties deviceProperties = physicalDevice->getDeviceProperties();

  const VkSamplerCreateInfo samplerCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.0f,
    .maxLod = VK_LOD_CLAMP_NONE,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };

  textureSampler = logicalDevice->createSampler(samplerCreateInfo);
}

void CubeMapTexture::createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths)
{
  int texWidth, texHeight, texChannels;
  std::array<stbi_uc*, 6> pixels{};

  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = stbi_load(paths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels[i])
    {
      throw std::runtime_error("failed to load texture image: " + paths[i]);
    }
  }

  const VkDeviceSize imageSize = texWidth * texHeight * 4;
  const VkDeviceSize totalSize = imageSize * paths.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [pixels, imageSize](void* data) {
    for (size_t i = 0; i < pixels.size(); ++i)
    {
      const VkDeviceSize offset = i * imageSize;
      memcpy(static_cast<uint8_t*>(data) + offset, pixels[i], imageSize);
      stbi_image_free(pixels[i]);
    }
  });

  Images::createImage(logicalDevice, physicalDevice, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, texWidth, texHeight, 1, 1,
                      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory,
                      VK_IMAGE_TYPE_2D, 6);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1, 6);

  // copy buffer to image

  std::vector<VkBufferImageCopy> bufferCopyRegions(6);
  for (uint32_t i = 0; i < 6; ++i)
  {
    bufferCopyRegions[i].bufferOffset = i * imageSize;
    bufferCopyRegions[i].bufferRowLength = 0;
    bufferCopyRegions[i].bufferImageHeight = 0;
    bufferCopyRegions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegions[i].imageSubresource.mipLevel = 0;
    bufferCopyRegions[i].imageSubresource.baseArrayLayer = i;
    bufferCopyRegions[i].imageSubresource.layerCount = 1;
    bufferCopyRegions[i].imageOffset = {0, 0, 0};
    bufferCopyRegions[i].imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
  }

  const auto commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice, commandPool);
  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
  Buffers::endSingleTimeCommands(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);

  // end copy buffer to image

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);

  textureImageView = Images::createImageView(logicalDevice, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_CUBE, 6);
}
