#ifndef CUBEMAPTEXTURE_H
#define CUBEMAPTEXTURE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <string>

class LogicalDevice;
class PhysicalDevice;

class CubeMapTexture {
public:
  CubeMapTexture(std::shared_ptr<LogicalDevice> logicalDevice, std::shared_ptr<PhysicalDevice> physicalDevice,
                 const VkCommandPool& commandPool, const std::array<std::string, 6>& paths);

  ~CubeMapTexture();

  [[nodiscard]] static VkDescriptorPoolSize getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT);

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

private:
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<PhysicalDevice> physicalDevice;

  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
  VkImageView textureImageView = VK_NULL_HANDLE;
  VkSampler textureSampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo imageInfo{};

  void createTextureSampler();

  void createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths);
};



#endif //CUBEMAPTEXTURE_H
