#ifndef VKE_IMAGES_H
#define VKE_IMAGES_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class LogicalDevice;
  class PhysicalDevice;

  namespace Images {

    struct ImageConfig {
      VkImageCreateFlags flags;
      VkExtent3D extent;
      uint32_t mipLevels;
      VkSampleCountFlagBits numSamples;
      VkFormat format;
      VkImageTiling tiling;
      VkImageUsageFlags usage;
      VkImageType imageType;
      uint32_t layerCount;
      VkMemoryPropertyFlags properties;
    };

    void createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const ImageConfig& imageConfig,
                     VkImage& image,
                     VkDeviceMemory& imageMemory);

    void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const VkCommandPool& commandPool,
                               VkImage image,
                               VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               uint32_t mipLevels,
                               uint32_t layerCount);

    void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const VkCommandPool& commandPool,
                           VkBuffer buffer,
                           VkImage image,
                           uint32_t width,
                           uint32_t height,
                           uint32_t depth);

    void copyImageToBuffer(const VkImage& image,
                           VkOffset3D offset,
                           VkExtent3D extent,
                           VkCommandBuffer commandBuffer,
                           VkBuffer stagingBuffer);

    VkImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                VkImage image,
                                VkFormat format,
                                VkImageAspectFlags aspectFlags,
                                uint32_t mipLevels,
                                VkImageViewType viewType,
                                uint32_t layerCount);

    bool hasStencilComponent(VkFormat format);
  }

} // namespace vke


#endif //VKE_IMAGES_H
