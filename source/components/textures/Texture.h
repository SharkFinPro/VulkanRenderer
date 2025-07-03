#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>

class LogicalDevice;

class Texture {
public:
  explicit Texture(const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual ~Texture();

  void init(const VkCommandPool& commandPool, const char* path, VkSamplerAddressMode addressMode);

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize() const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

  [[nodiscard]] ImTextureID getImGuiTexture();

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  uint32_t mipLevels;

  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
  VkImageView textureImageView = VK_NULL_HANDLE;
  VkSampler textureSampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo imageInfo{};

  VkDescriptorSet imGuiTexture = VK_NULL_HANDLE;

  virtual void createTextureImage(const VkCommandPool& commandPool, const char* path) = 0;

  void generateMipmaps(const VkCommandPool& commandPool, VkImage image, VkFormat imageFormat, int32_t texWidth,
                       int32_t texHeight, uint32_t mipLevels) const;

  void createTextureSampler(VkSamplerAddressMode addressMode);
};


#endif //VULKANPROJECT_TEXTURE_H
