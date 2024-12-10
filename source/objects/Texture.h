#ifndef VULKANPROJECT_TEXTURE_H
#define VULKANPROJECT_TEXTURE_H

#include <vulkan/vulkan.h>
#include <memory>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

class Texture {
public:
  Texture(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
          const VkCommandPool& commandPool, const char* path);
  ~Texture();

  [[nodiscard]] static VkDescriptorPoolSize getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT);

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  uint32_t mipLevels;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  VkDescriptorImageInfo imageInfo;

  void createTextureImage(const VkCommandPool& commandPool, const char* path);

  void generateMipmaps(const VkCommandPool& commandPool, VkImage image, VkFormat imageFormat, int32_t texWidth,
                       int32_t texHeight, uint32_t mipLevels) const;

  void createTextureSampler();
};


#endif //VULKANPROJECT_TEXTURE_H
