#ifndef VULKANPROJECT_IMAGERESOURCE_H
#define VULKANPROJECT_IMAGERESOURCE_H

#include <vulkan/vulkan_raii.hpp>
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
    vk::Extent2D extent;
    vk::CommandPool commandPool;
    vk::Format colorFormat = vk::Format::eUndefined;
    vk::Format depthFormat = vk::Format::eUndefined;
    vk::Format resolveFormat = vk::Format::eUndefined;
    vk::Format rayTracingFormat = vk::Format::eUndefined;
    vk::SampleCountFlagBits numSamples;
    vk::Sampler sampler = nullptr;
    bool isCubeMap = false;
  };

  class ImageResource final {
  public:
    explicit ImageResource(const ImageResourceConfig& config);

    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;

    ImageResource(ImageResource&&) noexcept = default;
    ImageResource& operator=(ImageResource&&) noexcept = default;

    ~ImageResource();

    [[nodiscard]] vk::Image getImage() const;

    [[nodiscard]] vk::ImageView getImageView() const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet() const;

    [[nodiscard]] const vk::DescriptorImageInfo& getDescriptorImageInfo() const;

  private:
    vk::raii::Image m_image = nullptr;
    vk::raii::ImageView m_imageView = nullptr;
    vk::raii::DeviceMemory m_imageMemory = nullptr;

    vk::DescriptorSet m_descriptorSet = nullptr;

    vk::DescriptorImageInfo m_descriptorImageInfo{};

    void createImage(const ImageResourceConfig& config);

    void createImageView(const ImageResourceConfig& config);

    void transitionImageLayout(const ImageResourceConfig& config) const;

    static vk::Format getFormat(const ImageResourceConfig& config);
  };
} // vke

#endif //VULKANPROJECT_IMAGERESOURCE_H