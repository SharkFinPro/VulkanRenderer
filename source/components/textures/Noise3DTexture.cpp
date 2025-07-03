#include "Noise3DTexture.h"
#include "../../utilities/Buffers.h"
#include "../../utilities/Images.h"
#include <stdexcept>
#include <cstdio>

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

Noise3DTexture::Noise3DTexture(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool)
  : Texture(logicalDevice)
{
  init(commandPool, nullptr, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void Noise3DTexture::createTextureImage(const VkCommandPool& commandPool, const char* path)
{
  mipLevels = 1;  // No mipmaps for noise

  int width, height, depth;
  const auto noiseData = ReadTexture3D("assets/noise/noise3d.064.tex", &width, &height, &depth);

  const VkDeviceSize imageSize = width * height * depth * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(m_logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [noiseData, imageSize](void* data) {
    memcpy(data, noiseData, imageSize);
  });

  delete noiseData;

  // Create a 3D m_logicalDevice
  Images::createImage(m_logicalDevice, 0, width, height, depth, mipLevels, VK_SAMPLE_COUNT_1_BIT,
                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, VK_IMAGE_TYPE_3D, 1);

  // Transition and copy buffer to 3D image
  Images::transitionImageLayout(m_logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, 1);
  Images::copyBufferToImage(m_logicalDevice, commandPool, stagingBuffer, textureImage, width, height, depth);

  Images::transitionImageLayout(m_logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, 1);

  Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);

  textureImageView = Images::createImageView(m_logicalDevice, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, VK_IMAGE_VIEW_TYPE_3D, 1);
}
