#include "TextureCubemap.h"
#include "../../components/core/logicalDevice/LogicalDevice.h"
#include "../../components/core/physicalDevice/PhysicalDevice.h"
#include "../../utilities/Buffers.h"
#include "../../utilities/Images.h"
#include <stb_image.h>
#include <stdexcept>

TextureCubemap::TextureCubemap(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const VkCommandPool& commandPool,
                               const std::array<std::string, 6>& paths)
  : Texture(logicalDevice, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
{
  createTextureImage(commandPool, paths);

  createImageView();
}

void TextureCubemap::createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths)
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

  const VkDeviceSize imageSize = texWidth * texHeight * 4;
  const VkDeviceSize totalSize = imageSize * paths.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(m_logicalDevice, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [pixels, imageSize](void* data) {
    for (size_t i = 0; i < pixels.size(); ++i)
    {
      const VkDeviceSize offset = i * imageSize;
      memcpy(static_cast<uint8_t*>(data) + offset, pixels[i], imageSize);
      stbi_image_free(pixels[i]);
    }
  });

  Images::createImage(m_logicalDevice, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, texWidth, texHeight, 1, 1,
                      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory,
                      VK_IMAGE_TYPE_2D, 6);

  Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1, 6);

  copyBufferToImage(commandPool, stagingBuffer, imageSize, texWidth, texHeight);

  Images::transitionImageLayout(m_logicalDevice, commandPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

  Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
}

void TextureCubemap::copyBufferToImage(const VkCommandPool& commandPool,
                                       VkBuffer stagingBuffer,
                                       const VkDeviceSize imageSize,
                                       const uint32_t textureWidth,
                                       const uint32_t textureHeight) const
{
  std::vector<VkBufferImageCopy> bufferCopyRegions(6);
  for (uint32_t i = 0; i < 6; ++i)
  {
    bufferCopyRegions[i].bufferOffset = i * imageSize;
    bufferCopyRegions[i].bufferRowLength = 0;
    bufferCopyRegions[i].bufferImageHeight = 0;
    bufferCopyRegions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegions[i].imageSubresource.mipLevel = 0;
    bufferCopyRegions[i].imageSubresource.baseArrayLayer = i;
    bufferCopyRegions[i].imageSubresource.layerCount = 1;
    bufferCopyRegions[i].imageOffset = { 0, 0, 0 };
    bufferCopyRegions[i].imageExtent = { textureWidth, textureHeight, 1 };
  }

  const auto commandBuffer = Buffers::beginSingleTimeCommands(m_logicalDevice, commandPool);
  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
  Buffers::endSingleTimeCommands(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), commandBuffer);
}

void TextureCubemap::createImageView()
{
  m_textureImageView = Images::createImageView(
    m_logicalDevice,
    m_textureImage,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_IMAGE_ASPECT_COLOR_BIT,
    1,
    VK_IMAGE_VIEW_TYPE_CUBE,
    6
  );

  m_imageInfo.imageView = m_textureImageView;
}
