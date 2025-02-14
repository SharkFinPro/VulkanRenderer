#include "CubeMapTexture.h"
#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"
#include "../utilities/Buffers.h"
#include "../utilities/Images.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <cmath>

CubeMapTexture::CubeMapTexture(std::shared_ptr<LogicalDevice> logicalDevice,
                               std::shared_ptr<PhysicalDevice> physicalDevice,
                               const VkCommandPool& commandPool,
                               const std::array<std::string, 6>& paths)
  : logicalDevice(std::move(logicalDevice)), physicalDevice(std::move(physicalDevice))
{
  createTextureImage(commandPool, paths);

  createTextureSampler();

  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = textureImageView;
  imageInfo.sampler = textureSampler;
}

CubeMapTexture::~CubeMapTexture()
{
  vkDestroySampler(logicalDevice->getDevice(), textureSampler, nullptr);
  vkDestroyImageView(logicalDevice->getDevice(), textureImageView, nullptr);

  vkDestroyImage(logicalDevice->getDevice(), textureImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), textureImageMemory, nullptr);
}

VkDescriptorPoolSize CubeMapTexture::getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT)
{
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

  return poolSize;
}

VkWriteDescriptorSet CubeMapTexture::getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const
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

void CubeMapTexture::createTextureSampler()
{
  VkPhysicalDeviceProperties deviceProperties{};
  vkGetPhysicalDeviceProperties(physicalDevice->getPhysicalDevice(), &deviceProperties);

  const VkSamplerCreateInfo samplerCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
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

  if (vkCreateSampler(logicalDevice->getDevice(), &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void CubeMapTexture::createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths)
{
  int texWidth, texHeight, texChannels;

  stbi_uc* pixels = stbi_load(paths[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  if (!pixels)
  {
    throw std::runtime_error("failed to load texture image!");
  }

  const VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  stbi_image_free(pixels);

  Images::createImage(logicalDevice, physicalDevice, texWidth, texHeight,
                      1, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, VK_IMAGE_TYPE_2D);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
  Images::copyBufferToImage(logicalDevice, commandPool, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                            static_cast<uint32_t>(texHeight), 1);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);

  textureImageView = Images::createImageView(logicalDevice, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_CUBE);
}
