#include "Noise3DTexture.h"
#include "../utilities/Buffers.h"
#include "../utilities/Images.h"
#include <stdexcept>

unsigned char* ReadTexture3D(const char* filename, int* width, int* height, int* depth)
{
  FILE *fp = fopen(filename, "rb");
  if( fp == nullptr )
  {
    throw std::runtime_error("Cannot find the file " + std::string(filename));
  }

  int nums, numt, nump;
  fread(&nums, 4, 1, fp);
  fread(&numt,  4, 1, fp);
  fread(&nump, 4, 1, fp);

  *width  = nums;
  *height = numt;
  *depth  = nump;

  auto* texture = new unsigned char[ 4 * nums * numt * nump ];
  fread(texture, 4 * nums * numt * nump, 1, fp);
  fclose(fp);

  return texture;
}

Noise3DTexture::Noise3DTexture(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                               const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const VkCommandPool& commandPool)
  : Texture(physicalDevice, logicalDevice)
{
  init(commandPool, nullptr);
}

void Noise3DTexture::createTextureImage(const VkCommandPool &commandPool, const char *path)
{
  mipLevels = 1;  // No mipmaps for noise

  int width, height, depth;
  auto noiseData = ReadTexture3D("assets/noise/noise3d.064.tex", &width, &height, &depth);

  VkDeviceSize imageSize = width * height * depth * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), imageSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, noiseData, imageSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  delete noiseData;

  // Create a 3D texture
  Images::createImage(logicalDevice, physicalDevice, width, height, depth, mipLevels, VK_SAMPLE_COUNT_1_BIT,
                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, VK_IMAGE_TYPE_3D);

  // Transition and copy buffer to 3D image
  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
  Images::copyBufferToImage(logicalDevice, commandPool, stagingBuffer, textureImage, width, height, depth);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

  vkDestroyBuffer(logicalDevice->getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), stagingBufferMemory, nullptr);

  textureImageView = Images::createImageView(logicalDevice, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, VK_IMAGE_VIEW_TYPE_3D);
}
