#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <vulkan/vulkan.h>

class Texture {
public:
  Texture(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue, const char* path);
  ~Texture();

  VkDescriptorImageInfo getImageInfo() const;

private:
  void createTextureImage(VkCommandPool& commandPool, VkQueue& graphicsQueue, const char* path);

  void generateMipmaps(VkCommandPool& commandPool, VkQueue& graphicsQueue, VkImage image, VkFormat imageFormat,
                       int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

  void createTextureImageView();

  void createTextureSampler();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  uint32_t mipLevels;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;
};


#endif //VULKANPROJECT_TEXTURE_H
