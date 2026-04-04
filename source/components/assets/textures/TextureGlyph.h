#ifndef VULKANPROJECT_TEXTUREGLYPH_H
#define VULKANPROJECT_TEXTUREGLYPH_H

#include "Texture.h"

namespace vke {

  class TextureGlyph final : public Texture {
  public:
    TextureGlyph(std::shared_ptr<LogicalDevice> logicalDevice,
                 const vk::raii::CommandPool& commandPool,
                 const unsigned char* pixelData,
                 uint32_t width,
                 uint32_t height);

  private:
    void createTextureImage(const vk::raii::CommandPool& commandPool,
                            const unsigned char* pixelData,
                            uint32_t width,
                            uint32_t height);

    void createAndFillStagingBuffer(const unsigned char* pixelData,
                                    uint32_t width,
                                    uint32_t height,
                                    vk::raii::Buffer& stagingBuffer,
                                    vk::raii::DeviceMemory& stagingBufferMemory) const;

    void createAndPrepareImage(const vk::raii::CommandPool& commandPool,
                               uint32_t width,
                               uint32_t height);

    void copyBufferToImage(const vk::raii::CommandPool& commandPool,
                           uint32_t width,
                           uint32_t height,
                           vk::raii::Buffer& stagingBuffer) const;

    void transitionImageToShaderReadable(const vk::raii::CommandPool& commandPool) const;

    void createImageView() override;
  };

} // vke

#endif //VULKANPROJECT_TEXTUREGLYPH_H