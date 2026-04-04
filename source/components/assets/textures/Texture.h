#ifndef VKE_TEXTURE_H
#define VKE_TEXTURE_H

#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace vke {

  class LogicalDevice;
  class SingleUseCommandBuffer;

  class Texture {
  public:
    Texture(std::shared_ptr<LogicalDevice> logicalDevice,
            vk::SamplerAddressMode samplerAddressMode);

    virtual ~Texture();

    [[nodiscard]] vk::WriteDescriptorSet getDescriptorSet(uint32_t binding,
                                                          const vk::DescriptorSet& dstSet) const;

    [[nodiscard]] ImTextureID getImGuiTexture();

    [[nodiscard]] vk::DescriptorImageInfo getImageInfo() const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::Image m_textureImage = nullptr;
    vk::raii::DeviceMemory m_textureImageMemory = nullptr;
    vk::raii::ImageView m_textureImageView = nullptr;
    vk::raii::Sampler m_textureSampler = nullptr;

    vk::DescriptorImageInfo m_imageInfo{};

    uint32_t m_mipLevels = 1;

    vk::DescriptorSet m_imGuiTexture{};

    void generateMipmaps(const vk::raii::CommandPool& commandPool,
                         vk::Image image,
                         vk::Format imageFormat,
                         int32_t texWidth,
                         int32_t texHeight,
                         uint32_t mipLevels) const;

    static void blitImage(const SingleUseCommandBuffer& commandBuffer,
                          vk::Image image,
                          uint32_t mipLevel,
                          int32_t mipWidth,
                          int32_t mipHeight);

    static void transitionMipLevelToTransferSrc(const SingleUseCommandBuffer& commandBuffer,
                                                vk::ImageMemoryBarrier& barrier,
                                                uint32_t mipLevel);

    static void transitionMipLevelToShaderRead(const SingleUseCommandBuffer& commandBuffer,
                                               vk::ImageMemoryBarrier& barrier);

    static void transitionFinalMipLevelToShaderRead(const SingleUseCommandBuffer& commandBuffer,
                                                    vk::ImageMemoryBarrier& barrier,
                                                    uint32_t mipLevel);

    void createTextureSampler(vk::SamplerAddressMode addressMode);

    virtual void createImageView() = 0;
  };

} // namespace vke

#endif //VKE_TEXTURE_H
