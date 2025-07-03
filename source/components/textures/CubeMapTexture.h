#ifndef CUBEMAPTEXTURE_H
#define CUBEMAPTEXTURE_H

#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <string>

class LogicalDevice;

class CubeMapTexture {
public:
  CubeMapTexture(const std::shared_ptr<LogicalDevice> &logicalDevice,
                 const VkCommandPool& commandPool,
                 const std::array<std::string, 6>& paths);

  ~CubeMapTexture();

  [[nodiscard]] static VkDescriptorPoolSize getDescriptorPoolSize(uint32_t MAX_FRAMES_IN_FLIGHT);

  [[nodiscard]] VkWriteDescriptorSet getDescriptorSet(uint32_t binding, const VkDescriptorSet& dstSet) const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
  VkImageView textureImageView = VK_NULL_HANDLE;
  VkSampler textureSampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo imageInfo{};

  void createTextureSampler();

  void createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths);
};



#endif //CUBEMAPTEXTURE_H
