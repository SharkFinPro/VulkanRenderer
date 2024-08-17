#ifndef VULKANPROJECT_IMAGES_H
#define VULKANPROJECT_IMAGES_H

#include <vulkan/vulkan.h>

class Images {
public:
  static void createImage(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height,
                          uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                          VkDeviceMemory& imageMemory);

  static void transitionImageLayout(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkImage image,
                                    VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

  static void copyBufferToImage(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer buffer,
                                VkImage image, uint32_t width, uint32_t height);

  static VkImageView createImageView(const VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

private:
  static bool hasStencilComponent(VkFormat format);
};


#endif //VULKANPROJECT_IMAGES_H
