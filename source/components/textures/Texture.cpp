#include "Texture.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../core/physicalDevice/PhysicalDevice.h"
#include "../../utilities/Buffers.h"
#include <backends/imgui_impl_vulkan.h>
#include <stdexcept>

Texture::Texture(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkSamplerAddressMode samplerAddressMode)
  : m_logicalDevice(logicalDevice), m_mipLevels(1)
{
  createTextureSampler(samplerAddressMode);

  m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

Texture::~Texture()
{
  if (m_imGuiTexture != VK_NULL_HANDLE)
  {
    ImGui_ImplVulkan_RemoveTexture(m_imGuiTexture);
  }

  m_logicalDevice->destroySampler(m_textureSampler);
  m_logicalDevice->destroyImageView(m_textureImageView);

  m_logicalDevice->destroyImage(m_textureImage);

  m_logicalDevice->freeMemory(m_textureImageMemory);
}

VkDescriptorPoolSize Texture::getDescriptorPoolSize() const
{
  const VkDescriptorPoolSize poolSize {
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = m_logicalDevice->getMaxFramesInFlight(),
  };

  return poolSize;
}

VkWriteDescriptorSet Texture::getDescriptorSet(const uint32_t binding, const VkDescriptorSet& dstSet) const
{
  const VkWriteDescriptorSet descriptorSet {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = dstSet,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &m_imageInfo
  };

  return descriptorSet;
}

ImTextureID Texture::getImGuiTexture()
{
  if (m_imGuiTexture == VK_NULL_HANDLE)
  {
    m_imGuiTexture = ImGui_ImplVulkan_AddTexture(m_textureSampler, m_textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  return reinterpret_cast<ImTextureID>(m_imGuiTexture);
}

void Texture::generateMipmaps(const VkCommandPool& commandPool, const VkImage image, const VkFormat imageFormat,
                              const int32_t texWidth, const int32_t texHeight, const uint32_t mipLevels) const
{
  const VkFormatProperties formatProperties = m_logicalDevice->getPhysicalDevice()->getFormatProperties(imageFormat);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  const VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(m_logicalDevice, commandPool);

  VkImageMemoryBarrier barrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    const VkImageBlit blit {
      .srcSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = i - 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .srcOffsets = {
        { 0, 0, 0 },
        { mipWidth, mipHeight, 1 }
      },
      .dstSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = i,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .dstOffsets = {
        { 0, 0, 0 },
        { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }
      }
    };

    vkCmdBlitImage(commandBuffer,
                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    if (mipWidth > 1)
    {
      mipWidth /= 2;
    }

    if (mipHeight > 1)
    {
      mipHeight /= 2;
    }
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  Buffers::endSingleTimeCommands(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), commandBuffer);
}

void Texture::createTextureSampler(const VkSamplerAddressMode addressMode)
{
  const VkPhysicalDeviceProperties deviceProperties = m_logicalDevice->getPhysicalDevice()->getDeviceProperties();

  const VkSamplerCreateInfo samplerCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = addressMode,
    .addressModeV = addressMode,
    .addressModeW = addressMode,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.0f,
    .maxLod = VK_LOD_CLAMP_NONE,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };

  m_textureSampler = m_logicalDevice->createSampler(samplerCreateInfo);

  m_imageInfo.sampler = m_textureSampler;
}
