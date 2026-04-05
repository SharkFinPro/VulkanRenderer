#include "TextureCubemap.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"
#include <stb_image.h>
#include <stdexcept>

namespace vke {

  TextureCubemap::TextureCubemap(std::shared_ptr<LogicalDevice> logicalDevice,
                                 const vk::raii::CommandPool& commandPool,
                                 const std::array<std::string, 6>& paths)
    : Texture(std::move(logicalDevice), vk::SamplerAddressMode::eClampToEdge)
  {
    createTextureImage(commandPool, paths);

    createImageView();
  }

  void TextureCubemap::createTextureImage(const vk::raii::CommandPool& commandPool,
                                          const std::array<std::string, 6>& paths)
  {
    int texWidth, texHeight;
    std::array<stbi_uc*, 6> pixels{};

    for (size_t i = 0; i < pixels.size(); ++i)
    {
      pixels[i] = stbi_load(paths[i].c_str(), &texWidth, &texHeight, nullptr, STBI_rgb_alpha);
      if (!pixels[i])
      {
        throw std::runtime_error("failed to load texture image: " + paths[i]);
      }
    }

    const vk::DeviceSize imageSize = texWidth * texHeight * 4;
    const vk::DeviceSize totalSize = imageSize * paths.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    Buffers::createBuffer(m_logicalDevice, totalSize, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [pixels, imageSize](void* data) {
      for (size_t i = 0; i < pixels.size(); ++i)
      {
        const vk::DeviceSize offset = i * imageSize;
        memcpy(static_cast<uint8_t*>(data) + offset, pixels[i], static_cast<size_t>(imageSize));
        stbi_image_free(pixels[i]);
      }
    });

    createImage(stagingBuffer, commandPool, imageSize, texWidth, texHeight);
  }

  void TextureCubemap::createImage(const vk::raii::Buffer& stagingBuffer,
                                   const vk::raii::CommandPool& commandPool,
                                   const vk::DeviceSize imageSize,
                                   const uint32_t texWidth,
                                   const uint32_t texHeight)
  {
    auto [image, imageMemory] = Images::createImage(
      m_logicalDevice,
      {
        vk::ImageCreateFlagBits::eCubeCompatible,
        vk::Extent3D{
          texWidth,
          texHeight,
          1
        },
        1,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageType::e2D,
        6,
        vk::MemoryPropertyFlagBits::eDeviceLocal
      }
    );

    m_textureImage = std::move(image);
    m_textureImageMemory = std::move(imageMemory);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, vk::Format::eR8G8B8A8Unorm,
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                  1, 6);

    copyBufferToImage(commandPool, stagingBuffer, imageSize, texWidth, texHeight);

    Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, vk::Format::eR8G8B8A8Unorm,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal, 1, 6);
  }

  void TextureCubemap::copyBufferToImage(const vk::raii::CommandPool& commandPool,
                                         const vk::raii::Buffer& stagingBuffer,
                                         const vk::DeviceSize imageSize,
                                         const uint32_t textureWidth,
                                         const uint32_t textureHeight) const
  {
    std::vector<vk::BufferImageCopy> bufferCopyRegions(6);
    for (uint32_t i = 0; i < 6; ++i)
    {
      bufferCopyRegions[i].bufferOffset = i * imageSize;
      bufferCopyRegions[i].bufferRowLength = 0;
      bufferCopyRegions[i].bufferImageHeight = 0;
      bufferCopyRegions[i].imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
      bufferCopyRegions[i].imageSubresource.mipLevel = 0;
      bufferCopyRegions[i].imageSubresource.baseArrayLayer = i;
      bufferCopyRegions[i].imageSubresource.layerCount = 1;
      bufferCopyRegions[i].imageOffset = vk::Offset3D{ 0, 0, 0 };
      bufferCopyRegions[i].imageExtent = vk::Extent3D{ textureWidth, textureHeight, 1 };
    }

    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([this, &commandBuffer, &stagingBuffer, bufferCopyRegions] {
      commandBuffer.copyBufferToImage(
        stagingBuffer,
        m_textureImage,
        vk::ImageLayout::eTransferDstOptimal,
        bufferCopyRegions
      );
    });
  }

  void TextureCubemap::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      vk::Format::eR8G8B8A8Unorm,
      vk::ImageAspectFlagBits::eColor,
      1,
      vk::ImageViewType::eCube,
      6
    );

    m_imageInfo.imageView = *m_textureImageView;
  }

} // namespace vke