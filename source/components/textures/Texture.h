#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class LogicalDevice;

class Texture {
public:
  Texture(const std::shared_ptr<LogicalDevice>& logicalDevice, VkSamplerAddressMode samplerAddressMode);

  virtual ~Texture();

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize() const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

  [[nodiscard]] ImTextureID getImGuiTexture();

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkImage m_textureImage = VK_NULL_HANDLE;
  VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
  VkImageView m_textureImageView = VK_NULL_HANDLE;
  VkSampler m_textureSampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo m_imageInfo{};

  uint32_t m_mipLevels;

  VkDescriptorSet m_imGuiTexture = VK_NULL_HANDLE;

  void generateMipmaps(const VkCommandPool& commandPool, VkImage image, VkFormat imageFormat, int32_t texWidth,
                       int32_t texHeight, uint32_t mipLevels) const;

  void createTextureSampler(VkSamplerAddressMode addressMode);

  virtual void createImageView() = 0;
};

} // namespace vke

#endif //VULKANPROJECT_TEXTURE_H
