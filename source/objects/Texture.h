#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <vulkan/vulkan.h>

class Texture {
public:
  Texture(VkDevice& device, VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const char* path);
  ~Texture();

  [[nodiscard]] VkDescriptorPoolSize getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT) const;

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

private:
  void createTextureImage(const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const char* path);

  void generateMipmaps(const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkImage image, VkFormat imageFormat,
                       int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;

  void createTextureSampler();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  uint32_t mipLevels;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  VkDescriptorImageInfo imageInfo;
};


#endif //VULKANPROJECT_TEXTURE_H
