#include "TextureGlyph.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"

namespace vke {

  TextureGlyph::TextureGlyph(std::shared_ptr<LogicalDevice> logicalDevice,
                             const vk::raii::CommandPool& commandPool,
                             const unsigned char* pixelData,
                             const uint32_t width,
                             const uint32_t height)
    : Texture(std::move(logicalDevice), vk::SamplerAddressMode::eClampToEdge)
  {
    createTextureImage(commandPool, pixelData, width, height);

    createImageView();
  }

  void TextureGlyph::createTextureImage(const vk::raii::CommandPool& commandPool,
                                        const unsigned char* pixelData,
                                        const uint32_t width,
                                        const uint32_t height)
  {
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    createAndFillStagingBuffer(pixelData, width, height, stagingBuffer, stagingBufferMemory);

    createAndPrepareImage(commandPool, width, height);

    copyBufferToImage(commandPool, width, height, stagingBuffer);

    transitionImageToShaderReadable(commandPool);
  }

  void TextureGlyph::createAndFillStagingBuffer(const unsigned char* pixelData,
                                                const uint32_t width,
                                                const uint32_t height,
                                                vk::raii::Buffer& stagingBuffer,
                                                vk::raii::DeviceMemory& stagingBufferMemory) const
  {
    const auto imageSize = width * height;

    Buffers::createBuffer(
      m_logicalDevice,
      imageSize,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer,
      stagingBufferMemory
    );

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [pixelData, imageSize](void* data) {
      memcpy(data, pixelData, imageSize);
    });
  }

  void TextureGlyph::createAndPrepareImage(const vk::raii::CommandPool& commandPool,
                                           const uint32_t width,
                                           const uint32_t height)
  {
    auto [ image, imageMemory ] = Images::createImage(
      m_logicalDevice,
      {
        {},
        vk::Extent3D{
          width,
          height,
          1,
        },
        m_mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8Unorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageType::e2D,
        1,
        vk::MemoryPropertyFlagBits::eDeviceLocal
      }
    );

    m_textureImage = std::move(image);
    m_textureImageMemory = std::move(imageMemory);

    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_textureImage,
      vk::Format::eR8Unorm,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eTransferDstOptimal,
      m_mipLevels,
      1
    );
  }

  void TextureGlyph::copyBufferToImage(const vk::raii::CommandPool& commandPool,
                                       const uint32_t width,
                                       const uint32_t height,
                                       vk::raii::Buffer& stagingBuffer) const
  {
    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([this, commandBuffer, width, height, &stagingBuffer] {
      const vk::BufferImageCopy region{
        0,
        0,
        0,
        vk::ImageSubresourceLayers{
          vk::ImageAspectFlagBits::eColor,
          0,
          0,
          1,
        },
        vk::Offset3D{0, 0, 0},
        vk::Extent3D{width, height, 1}
      };

      commandBuffer.copyBufferToImage(
        stagingBuffer,
        m_textureImage,
        vk::ImageLayout::eTransferDstOptimal,
        { region }
      );
    });
  }

  void TextureGlyph::transitionImageToShaderReadable(const vk::raii::CommandPool& commandPool) const
  {
    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_textureImage,
      vk::Format::eR8Unorm,
      vk::ImageLayout::eTransferDstOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      m_mipLevels,
      1
    );
  }

  void TextureGlyph::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      vk::Format::eR8Unorm,
      vk::ImageAspectFlagBits::eColor,
      m_mipLevels,
      vk::ImageViewType::e2D,
      1
    );

    m_imageInfo.imageView = *m_textureImageView;
  }

} // vke