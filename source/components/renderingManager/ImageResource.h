#ifndef VULKANPROJECT_IMAGERESOURCE_H
#define VULKANPROJECT_IMAGERESOURCE_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class LogicalDevice;

  enum class ImageResourceType {
    Color,
    Depth,
    Resolve,
    RayTracingOutput,
    Undefined
  };

  struct ImageResourceConfig {
    ImageResourceType imageResourceType = ImageResourceType::Undefined;
    std::shared_ptr<LogicalDevice> logicalDevice;
    VkExtent2D extent;
    VkCommandPool commandPool;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkFormat resolveFormat = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits numSamples;
    VkSampler sampler = VK_NULL_HANDLE;
    bool isCubeMap = false;
  };

  class ImageResource final {
  public:
    explicit ImageResource(ImageResourceConfig config);

    ~ImageResource();

    [[nodiscard]] VkImage getImage() const;

    [[nodiscard]] VkImageView getImageView() const;

    [[nodiscard]] VkDescriptorSet getDescriptorSet() const;

    [[nodiscard]] const VkDescriptorImageInfo& getDescriptorImageInfo() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;

    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkDescriptorImageInfo m_descriptorImageInfo{};

    void createImage(const ImageResourceConfig& config);

    void createImageView(const ImageResourceConfig& config);

    void transitionImageLayout(const ImageResourceConfig& config) const;

    static VkFormat getFormat(const ImageResourceConfig& config);
  };
} // vke

#endif //VULKANPROJECT_IMAGERESOURCE_H