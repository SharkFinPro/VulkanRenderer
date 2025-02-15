#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <vulkan/vulkan.h>
#include <memory>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include <imgui.h>

class Texture {
public:
  Texture(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice);

  virtual ~Texture();

  void init(const VkCommandPool& commandPool, const char* path, VkSamplerAddressMode addressMode);

  [[nodiscard]] static VkDescriptorPoolSize getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT);

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

  [[nodiscard]] ImTextureID getImGuiTexture();

protected:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  uint32_t mipLevels;

  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
  VkImageView textureImageView = VK_NULL_HANDLE;
  VkSampler textureSampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo imageInfo{};

  VkDescriptorSet imGuiTexture = VK_NULL_HANDLE;

  virtual void createTextureImage(const VkCommandPool& commandPool, const char* path);

  void generateMipmaps(const VkCommandPool& commandPool, VkImage image, VkFormat imageFormat, int32_t texWidth,
                       int32_t texHeight, uint32_t mipLevels) const;

  void createTextureSampler(VkSamplerAddressMode addressMode);
};


#endif //VULKANPROJECT_TEXTURE_H
