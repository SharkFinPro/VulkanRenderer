#ifndef VKE_TEXTURE_H
#define VKE_TEXTURE_H

#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class LogicalDevice;

  class Texture {
  public:
    Texture(std::shared_ptr<LogicalDevice> logicalDevice,
            VkSamplerAddressMode samplerAddressMode);

    virtual ~Texture();

    [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding,
                                                        const VkDescriptorSet& dstSet) const;

    [[nodiscard]] ImTextureID getImGuiTexture();

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    VkDescriptorImageInfo m_imageInfo{};

    uint32_t m_mipLevels = 1;

    VkDescriptorSet m_imGuiTexture = VK_NULL_HANDLE;

    void generateMipmaps(const VkCommandPool& commandPool,
                         VkImage image,
                         VkFormat imageFormat,
                         int32_t texWidth,
                         int32_t texHeight,
                         uint32_t mipLevels) const;

    static void blitImage(VkCommandBuffer commandBuffer,
                          VkImage image,
                          uint32_t mipLevel,
                          int32_t mipWidth,
                          int32_t mipHeight);

    static void transitionMipLevelToTransferSrc(VkCommandBuffer commandBuffer,
                                                VkImageMemoryBarrier& barrier,
                                                uint32_t mipLevel);

    static void transitionMipLevelToShaderRead(VkCommandBuffer commandBuffer,
                                               VkImageMemoryBarrier& barrier);

    static void transitionFinalMipLevelToShaderRead(VkCommandBuffer commandBuffer,
                                                    VkImageMemoryBarrier& barrier,
                                                    uint32_t mipLevel);

    void createTextureSampler(VkSamplerAddressMode addressMode);

    virtual void createImageView() = 0;
  };

} // namespace vke

#endif //VKE_TEXTURE_H
