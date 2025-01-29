#include "Noise3DTexture.h"
#include "../utilities/Buffers.h"
#include "../utilities/Images.h"

constexpr uint32_t RESOLUTION = 250;

constexpr uint32_t WIDTH = RESOLUTION;
constexpr uint32_t HEIGHT = RESOLUTION;
constexpr uint32_t DEPTH = RESOLUTION;

Noise3DTexture::Noise3DTexture(const std::shared_ptr<PhysicalDevice> &physicalDevice,
                               const std::shared_ptr<LogicalDevice> &logicalDevice, const VkCommandPool& commandPool)
  : Texture(physicalDevice, logicalDevice)
{
  init(commandPool, nullptr);
}

void Noise3DTexture::createTextureImage(const VkCommandPool &commandPool, const char *path)
{
  mipLevels = 1;  // No mipmaps for noise

  // Generate 3D noise data
  std::vector<uint8_t> noiseData(WIDTH * HEIGHT * DEPTH * 4); // RGBA8 format

#pragma omp parallel for
  for (uint32_t z = 0; z < DEPTH; ++z)
  {
      for (uint32_t y = 0; y < HEIGHT; ++y)
      {
          for (uint32_t x = 0; x < WIDTH; ++x)
          {
              size_t index = (z * WIDTH * HEIGHT + y * WIDTH + x) * 4;
              uint8_t value = static_cast<uint8_t>(rand() % 256);  // Simple noise
              noiseData[index] = value;
              noiseData[index + 1] = value;
              noiseData[index + 2] = value;
              noiseData[index + 3] = 255;  // Alpha
          }
      }
  }

  VkDeviceSize imageSize = noiseData.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), imageSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, noiseData.data(), imageSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  // Create a 3D texture
  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), WIDTH, HEIGHT, DEPTH, mipLevels,
                      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, VK_IMAGE_TYPE_3D);

  // Transition and copy buffer to 3D image
  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
  Images::copyBufferToImage(logicalDevice, commandPool, stagingBuffer, textureImage, WIDTH, HEIGHT, DEPTH);

  Images::transitionImageLayout(logicalDevice, commandPool, textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

  vkDestroyBuffer(logicalDevice->getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), stagingBufferMemory, nullptr);

  textureImageView = Images::createImageView(logicalDevice->getDevice(), textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, VK_IMAGE_VIEW_TYPE_3D);
}
