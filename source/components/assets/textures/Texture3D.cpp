#include "Texture3D.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"
#include <stdexcept>
#include <cstdio>

namespace vke {

  unsigned char* ReadTexture3D(const char* filename,
                               int* width,
                               int* height,
                               int* depth)
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

  Texture3D::Texture3D(std::shared_ptr<LogicalDevice> logicalDevice,
                       const VkCommandPool& commandPool,
                       const char* path,
                       const VkSamplerAddressMode samplerAddressMode)
    : Texture(std::move(logicalDevice), samplerAddressMode)
  {
    createTextureImage(commandPool, path);

    createImageView();
  }

  void Texture3D::createTextureImage(const VkCommandPool& commandPool,
                                     const char* path)
  {
    m_mipLevels = 1;

    int width, height, depth;
    const auto imageData = ReadTexture3D(path, &width, &height, &depth);

    const VkDeviceSize imageSize = width * height * depth * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Buffers::createBuffer(m_logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [imageData, imageSize](void* data) {
      memcpy(data, imageData, imageSize);
    });

    delete imageData;

    Images::createImage(
      m_logicalDevice,
      0,
      {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = static_cast<uint32_t>(depth),
      },
      m_mipLevels,
      VK_SAMPLE_COUNT_1_BIT,
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      m_textureImage,
      m_textureImageMemory,
      VK_IMAGE_TYPE_3D,
      1
    );

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, 1);
    Images::copyBufferToImage(m_logicalDevice, commandPool, stagingBuffer, m_textureImage, width, height, depth);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels, 1);

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void Texture3D::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_ASPECT_COLOR_BIT,
      m_mipLevels,
      VK_IMAGE_VIEW_TYPE_3D,
      1
    );

    m_imageInfo.imageView = m_textureImageView;
  }

} // namespace vke