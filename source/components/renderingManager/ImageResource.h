#ifndef VULKANPROJECT_IMAGERESOURCE_H
#define VULKANPROJECT_IMAGERESOURCE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

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
    uint32_t layerCount = 1; // Above 1 makes a 2D array image; ignored for cube maps, which have 6
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

    // A 2D view of one face of a cube map image (face < 6), for rendering into that face alone
    [[nodiscard]] vk::ImageView getFaceImageView(uint32_t face) const;

    // A 2D view of one layer of a cube map or array image, for rendering into that layer alone
    [[nodiscard]] vk::ImageView getLayerImageView(uint32_t layer) const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet() const;

    [[nodiscard]] const vk::DescriptorImageInfo& getDescriptorImageInfo() const;

  private:
    vk::raii::Image m_image = nullptr;
    vk::raii::ImageView m_imageView = nullptr;
    std::vector<vk::raii::ImageView> m_layerImageViews;
    vk::raii::DeviceMemory m_imageMemory = nullptr;

    vk::DescriptorSet m_descriptorSet = nullptr;

    vk::DescriptorImageInfo m_descriptorImageInfo{};

    void createImage(const ImageResourceConfig& config);

    void createImageView(const ImageResourceConfig& config);

    void transitionImageLayout(const ImageResourceConfig& config) const;

    static vk::Format getFormat(const ImageResourceConfig& config);

    static uint32_t getLayerCount(const ImageResourceConfig& config);
  };
} // vke

#endif //VULKANPROJECT_IMAGERESOURCE_H