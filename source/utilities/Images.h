#ifndef VKE_IMAGES_H
#define VKE_IMAGES_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class SingleUseCommandBuffer;

  namespace Images {

    struct ImageConfig {
      vk::ImageCreateFlags flags;
      vk::Extent3D extent;
      uint32_t mipLevels;
      vk::SampleCountFlagBits numSamples;
      vk::Format format;
      vk::ImageTiling tiling;
      vk::ImageUsageFlags usage;
      vk::ImageType imageType;
      uint32_t layerCount;
      vk::MemoryPropertyFlags properties;
    };

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                                   const ImageConfig& imageConfig);

    // Recording-only variants: no submit, no wait. Use these to batch several transitions and
    // copies into one command buffer instead of paying a queue drain for each.
    void recordImageLayoutTransition(const CommandBuffer& commandBuffer,
                                     vk::Image image,
                                     vk::Format format,
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     uint32_t mipLevels,
                                     uint32_t layerCount);

    void recordCopyBufferToImage(const CommandBuffer& commandBuffer,
                                 vk::Buffer buffer,
                                 vk::Image image,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth);

    void transitionImageLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               vk::CommandPool commandPool,
                               vk::Image image,
                               vk::Format format,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               uint32_t mipLevels,
                               uint32_t layerCount);

    void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           vk::CommandPool commandPool,
                           vk::Buffer buffer,
                           vk::Image image,
                           uint32_t width,
                           uint32_t height,
                           uint32_t depth);

    void copyImageToBuffer(vk::Image image,
                           vk::Offset3D offset,
                           vk::Extent3D extent,
                           const CommandBuffer& commandBuffer,
                           vk::Buffer stagingBuffer);

    vk::raii::ImageView createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                        vk::Image image,
                                        vk::Format format,
                                        vk::ImageAspectFlags aspectFlags,
                                        uint32_t mipLevels,
                                        vk::ImageViewType viewType,
                                        uint32_t layerCount);

    bool hasStencilComponent(vk::Format format);

  } // namespace Images

} // namespace vke

#endif //VKE_IMAGES_H
