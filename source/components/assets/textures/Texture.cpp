#include "Texture.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <backends/imgui_impl_vulkan.h>
#include <stdexcept>

namespace vke {

  Texture::Texture(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const vk::SamplerAddressMode samplerAddressMode)
  {
    createTextureSampler(logicalDevice, samplerAddressMode);

    m_imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  }

  Texture::~Texture()
  {
    if (m_imGuiTexture)
    {
      ImGui_ImplVulkan_RemoveTexture(m_imGuiTexture);
    }
  }

  vk::WriteDescriptorSet Texture::getDescriptorSet(const uint32_t binding,
                                                   const vk::DescriptorSet& dstSet) const
  {
    return vk::WriteDescriptorSet{
      .dstSet = dstSet,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &m_imageInfo,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
    };
  }

  ImTextureID Texture::getImGuiTexture()
  {
    if (!m_imGuiTexture)
    {
      m_imGuiTexture = ImGui_ImplVulkan_AddTexture(
        *m_textureSampler,
        *m_textureImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      );
    }

    return reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(m_imGuiTexture));
  }

  vk::DescriptorImageInfo Texture::getImageInfo() const
  {
    return m_imageInfo;
  }

  void Texture::generateMipmaps(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                const vk::CommandPool commandPool,
                                const vk::Image image,
                                const vk::Format imageFormat,
                                const int32_t texWidth,
                                const int32_t texHeight,
                                const uint32_t mipLevels)
  {
    const auto formatProperties = logicalDevice->getPhysicalDevice()->getFormatProperties(imageFormat);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
      throw std::runtime_error("texture image format does not support linear blitting!");
    }

    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, image, texWidth, texHeight, mipLevels] {
      vk::ImageMemoryBarrier barrier {
        .sType = vk::StructureType::eImageMemoryBarrier,
        .srcAccessMask = {},
        .dstAccessMask = {},
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eUndefined,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
          vk::ImageAspectFlagBits::eColor,
          0, 1,
          0, 1
        }
      };

      int32_t mipWidth = texWidth;
      int32_t mipHeight = texHeight;

      for (uint32_t i = 1; i < mipLevels; i++)
      {
        transitionMipLevelToTransferSrc(commandBuffer, barrier, i - 1);

        blitImage(commandBuffer, image, i - 1, mipWidth, mipHeight);

        transitionMipLevelToShaderRead(commandBuffer, barrier);

        if (mipWidth > 1)
        {
          mipWidth /= 2;
        }

        if (mipHeight > 1)
        {
          mipHeight /= 2;
        }
      }

      transitionFinalMipLevelToShaderRead(commandBuffer, barrier, mipLevels - 1);
    });
  }

  void Texture::blitImage(const SingleUseCommandBuffer& commandBuffer,
                          const vk::Image image,
                          const uint32_t mipLevel,
                          const int32_t mipWidth,
                          const int32_t mipHeight)
  {
    const vk::ImageBlit blit{
      vk::ImageSubresourceLayers{
        vk::ImageAspectFlagBits::eColor,
        mipLevel,
        0,
        1
      },
      std::array{
        vk::Offset3D{0, 0, 0},
        vk::Offset3D{mipWidth, mipHeight, 1}
      },
      vk::ImageSubresourceLayers{
        vk::ImageAspectFlagBits::eColor,
        mipLevel + 1,
        0,
        1
      },
      std::array{
        vk::Offset3D{0, 0, 0},
        vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
      }
    };

    commandBuffer.blitImage(
      image,
      vk::ImageLayout::eTransferSrcOptimal,
      image,
      vk::ImageLayout::eTransferDstOptimal,
      { blit },
      vk::Filter::eLinear
    );
  }

  void Texture::transitionMipLevelToTransferSrc(const SingleUseCommandBuffer& commandBuffer,
                                                vk::ImageMemoryBarrier& barrier,
                                                const uint32_t mipLevel)
  {
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eTransfer,
      {},
      {},
      {},
      { barrier }
    );
  }

  void Texture::transitionMipLevelToShaderRead(const SingleUseCommandBuffer& commandBuffer,
                                               vk::ImageMemoryBarrier& barrier)
  {
    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eFragmentShader,
      {},
      {},
      {},
      { barrier }
    );
  }

  void Texture::transitionFinalMipLevelToShaderRead(const SingleUseCommandBuffer& commandBuffer,
                                                    vk::ImageMemoryBarrier& barrier,
                                                    const uint32_t mipLevel)
  {
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eFragmentShader,
      {},
      {},
      {},
      { barrier }
    );
  }

  void Texture::createTextureSampler(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                     const vk::SamplerAddressMode addressMode)
  {
    const vk::PhysicalDeviceProperties deviceProperties = logicalDevice->getPhysicalDevice()->getDeviceProperties();

    const vk::SamplerCreateInfo samplerCreateInfo{
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = addressMode,
      .addressModeV = addressMode,
      .addressModeW = addressMode,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::True,
      .maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy,
      .compareEnable = vk::False,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = vk::LodClampNone,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = vk::False
    };

    m_textureSampler = logicalDevice->createSampler(samplerCreateInfo);

    m_imageInfo.sampler = *m_textureSampler;
  }

} // namespace vke