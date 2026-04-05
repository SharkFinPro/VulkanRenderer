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

  Texture3D::Texture3D(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const vk::raii::CommandPool& commandPool,
                       const char* path,
                       const vk::SamplerAddressMode samplerAddressMode)
    : Texture(logicalDevice, samplerAddressMode)
  {
    createTextureImage(logicalDevice, commandPool, path);

    createImageView(logicalDevice);
  }

  void Texture3D::createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                     const vk::raii::CommandPool& commandPool,
                                     const char* path)
  {
    m_mipLevels = 1;

    int width, height, depth;
    const auto imageData = ReadTexture3D(path, &width, &height, &depth);

    const vk::DeviceSize imageSize = width * height * depth * 4;

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    Buffers::createBuffer(logicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [imageData, imageSize](void* data) {
      memcpy(data, imageData, static_cast<size_t>(imageSize));
    });

    delete[] imageData;

    auto [image, imageMemory] = Images::createImage(
      logicalDevice,
      {
        {},
        vk::Extent3D{
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height),
          static_cast<uint32_t>(depth),
        },
        m_mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageType::e3D,
        1,
        vk::MemoryPropertyFlagBits::eDeviceLocal
      }
    );

    m_textureImage = std::move(image);
    m_textureImageMemory = std::move(imageMemory);

    Images::transitionImageLayout(logicalDevice, commandPool, m_textureImage, vk::Format::eR8G8B8A8Unorm,
      vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_mipLevels, 1);

    Images::copyBufferToImage(logicalDevice, commandPool, stagingBuffer, m_textureImage, width, height, depth);

    Images::transitionImageLayout(logicalDevice, commandPool, m_textureImage, vk::Format::eR8G8B8A8Unorm,
      vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, m_mipLevels, 1);
  }

  void Texture3D::createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    m_textureImageView = Images::createImageView(
      logicalDevice,
      m_textureImage,
      vk::Format::eR8G8B8A8Unorm,
      vk::ImageAspectFlagBits::eColor,
      m_mipLevels,
      vk::ImageViewType::e3D,
      1
    );

    m_imageInfo.imageView = *m_textureImageView;
  }

} // namespace vke