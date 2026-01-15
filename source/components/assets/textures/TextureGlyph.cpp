#include "TextureGlyph.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"

namespace vke {

  TextureGlyph::TextureGlyph(std::shared_ptr<LogicalDevice> logicalDevice,
                             const VkCommandPool& commandPool,
                             const unsigned char* pixelData,
                             const uint32_t width,
                             const uint32_t height)
    : Texture(std::move(logicalDevice), VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
  {
    createTextureImage(commandPool, pixelData, width, height);

    createImageView();
  }

  void TextureGlyph::createTextureImage(const VkCommandPool& commandPool,
                                        const unsigned char* pixelData,
                                        const uint32_t width,
                                        const uint32_t height)
  {
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    createAndFillStagingBuffer(pixelData, width, height, stagingBuffer, stagingBufferMemory);

    createAndPrepareImage(commandPool, width, height);

    copyBufferToImage(commandPool, width, height, stagingBuffer);

    transitionImageToShaderReadable(commandPool);

    cleanupStagingBuffer(stagingBuffer, stagingBufferMemory);
  }

  void TextureGlyph::createAndFillStagingBuffer(const unsigned char* pixelData,
                                                const uint32_t width,
                                                const uint32_t height,
                                                VkBuffer& stagingBuffer,
                                                VkDeviceMemory& stagingBufferMemory) const
  {
    const auto imageSize = width * height;

    Buffers::createBuffer(
      m_logicalDevice,
      imageSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory
    );

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [pixelData, imageSize](void* data) {
      memcpy(data, pixelData, imageSize);
    });
  }

  void TextureGlyph::createAndPrepareImage(const VkCommandPool& commandPool,
                                           const uint32_t width,
                                           const uint32_t height)
  {
    Images::createImage(
      m_logicalDevice,
      {
        .flags = 0,
        .extent = {
          .width = width,
          .height = height,
          .depth = 1,
        },
        .mipLevels = m_mipLevels,
        .numSamples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R8_UNORM,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .layerCount = 1,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      },
      m_textureImage,
      m_textureImageMemory
    );

    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_textureImage,
      VK_FORMAT_R8_UNORM,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      m_mipLevels,
      1
    );
  }

  void TextureGlyph::copyBufferToImage(const VkCommandPool& commandPool,
                                       const uint32_t width,
                                       const uint32_t height,
                                       const VkBuffer& stagingBuffer) const
  {
    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([this, commandBuffer, width, height, stagingBuffer] {
      const VkBufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}
      };

      commandBuffer.copyBufferToImage(
        stagingBuffer,
        m_textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        { region }
      );
    });
  }

  void TextureGlyph::transitionImageToShaderReadable(const VkCommandPool& commandPool) const
  {
    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_textureImage,
      VK_FORMAT_R8_UNORM,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      m_mipLevels,
      1
    );
  }

  void TextureGlyph::cleanupStagingBuffer(VkBuffer& stagingBuffer,
                                          VkDeviceMemory& stagingBufferMemory) const
  {
    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void TextureGlyph::createImageView()
  {
    m_textureImageView = Images::createImageView(
      m_logicalDevice,
      m_textureImage,
      VK_FORMAT_R8_UNORM,
      VK_IMAGE_ASPECT_COLOR_BIT,
      m_mipLevels,
      VK_IMAGE_VIEW_TYPE_2D,
      1
    );

    m_imageInfo.imageView = m_textureImageView;
  }

} // vke