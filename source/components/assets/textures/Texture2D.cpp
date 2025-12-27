#include "Texture2D.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>
#include <cmath>
#include <stdexcept>

namespace vke {

  Texture2D::Texture2D(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const VkCommandPool& commandPool,
                       const char* path,
                       const VkSamplerAddressMode samplerAddressMode)
    : Texture(logicalDevice, samplerAddressMode)
  {
    createTextureImage(commandPool, path);

    createImageView();
  }

  void Texture2D::createTextureImage(const VkCommandPool& commandPool, const char* path)
  {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
      throw std::runtime_error("failed to load texture image!");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    const VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Buffers::createBuffer(m_logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [pixels, imageSize](void* data) {
      memcpy(data, pixels, imageSize);
    });

    stbi_image_free(pixels);

    Images::createImage(m_logicalDevice, 0, texWidth, texHeight,
                        1, m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory, VK_IMAGE_TYPE_2D, 1);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, 1);
    Images::copyBufferToImage(m_logicalDevice, commandPool, stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth),
                              static_cast<uint32_t>(texHeight), 1);
    // Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);

    generateMipmaps(commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, m_mipLevels);
  }

  void Texture2D::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_ASPECT_COLOR_BIT,
      m_mipLevels,
      VK_IMAGE_VIEW_TYPE_2D,
      1
    );

    m_imageInfo.imageView = m_textureImageView;
  }

} // namespace vke