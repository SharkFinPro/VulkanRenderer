#ifndef VULKANPROJECT_IMAGES_H
#define VULKANPROJECT_IMAGES_H

#include <vulkan/vulkan.h>
#include <memory>

class LogicalDevice;
class PhysicalDevice;

namespace Images {
  void createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const std::shared_ptr<PhysicalDevice>& physicalDevice, VkImageCreateFlags flags,
                   uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                   VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                   VkImage& image, VkDeviceMemory& imageMemory, VkImageType imageType, uint32_t layerCount);

  void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels, uint32_t layerCount);

  void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                         VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

  void copyImageToBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkImage& image, VkOffset3D offset,
                         VkExtent3D extent, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer);

  VkImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice, VkImage image, VkFormat format,
                              VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType viewType,
                              uint32_t layerCount);

  bool hasStencilComponent(VkFormat format);
}


#endif //VULKANPROJECT_IMAGES_H
