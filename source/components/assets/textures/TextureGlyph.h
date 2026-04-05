#ifndef VULKANPROJECT_TEXTUREGLYPH_H
#define VULKANPROJECT_TEXTUREGLYPH_H

#include "Texture.h"

namespace vke {

  class TextureGlyph final : public Texture {
  public:
    TextureGlyph(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 vk::CommandPool commandPool,
                 const unsigned char* pixelData,
                 uint32_t width,
                 uint32_t height);

  private:
    void createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            vk::CommandPool commandPool,
                            const unsigned char* pixelData,
                            uint32_t width,
                            uint32_t height);

    static void createAndFillStagingBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const unsigned char* pixelData,
                                           uint32_t width,
                                           uint32_t height,
                                           vk::raii::Buffer& stagingBuffer,
                                           vk::raii::DeviceMemory& stagingBufferMemory);

    void createAndPrepareImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               vk::CommandPool commandPool,
                               uint32_t width,
                               uint32_t height);

    void copyBufferToImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           vk::CommandPool commandPool,
                           uint32_t width,
                           uint32_t height,
                           vk::raii::Buffer& stagingBuffer) const;

    void transitionImageToShaderReadable(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         vk::CommandPool commandPool) const;

    void createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice) override;
  };

} // vke

#endif //VULKANPROJECT_TEXTUREGLYPH_H