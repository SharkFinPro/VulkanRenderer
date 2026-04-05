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

  Texture2D::Texture2D(std::shared_ptr<LogicalDevice> logicalDevice,
                       const vk::CommandPool commandPool,
                       const char* path,
                       const vk::SamplerAddressMode samplerAddressMode)
    : Texture(std::move(logicalDevice), samplerAddressMode)
  {
    createTextureImage(commandPool, path);

    createImageView();
  }

  void Texture2D::createTextureImage(const vk::CommandPool commandPool,
                                     const char* path)
  {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
      throw std::runtime_error("failed to load texture image!");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    const vk::DeviceSize imageSize = texWidth * texHeight * 4;

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    Buffers::createBuffer(m_logicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [pixels, imageSize](void* data) {
      memcpy(data, pixels, static_cast<size_t>(imageSize));
    });

    stbi_image_free(pixels);

    auto [image, imageMemory] = Images::createImage(
      m_logicalDevice,
      {
        {},
        vk::Extent3D{
          static_cast<uint32_t>(texWidth),
          static_cast<uint32_t>(texHeight),
          1,
        },
        m_mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageType::e2D,
        1,
        vk::MemoryPropertyFlagBits::eDeviceLocal
      }
    );

    m_textureImage = std::move(image);
    m_textureImageMemory = std::move(imageMemory);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, vk::Format::eR8G8B8A8Unorm,
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_mipLevels, 1);
    Images::copyBufferToImage(m_logicalDevice, commandPool, stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth),
                              static_cast<uint32_t>(texHeight), 1);
    // Transitioned to vk::ImageLayout::eShaderReadOnlyOptimal while generating mipmaps

    generateMipmaps(commandPool, *m_textureImage, vk::Format::eR8G8B8A8Unorm, texWidth, texHeight, m_mipLevels);
  }

  void Texture2D::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      vk::Format::eR8G8B8A8Unorm,
      vk::ImageAspectFlagBits::eColor,
      m_mipLevels,
      vk::ImageViewType::e2D,
      1
    );

    m_imageInfo.imageView = *m_textureImageView;
  }

} // namespace vke