#ifndef VULKANPROJECT_TEXTUREGLYPH_H
#define VULKANPROJECT_TEXTUREGLYPH_H

#include "Texture.h"

namespace vke {

  class TextureGlyph final : public Texture {
  public:
    TextureGlyph(std::shared_ptr<LogicalDevice> logicalDevice,
                 const VkCommandPool& commandPool,
                 const unsigned char* pixelData,
                 uint32_t width,
                 uint32_t height);

  private:
    void createTextureImage(const VkCommandPool& commandPool,
                            const unsigned char* pixelData,
                            uint32_t width,
                            uint32_t height);

    void createAndFillStagingBuffer(const unsigned char* pixelData,
                                    uint32_t width,
                                    uint32_t height,
                                    VkBuffer& stagingBuffer,
                                    VkDeviceMemory& stagingBufferMemory) const;

    void createAndPrepareImage(const VkCommandPool& commandPool,
                               uint32_t width,
                               uint32_t height);

    void copyBufferToImage(const VkCommandPool& commandPool,
                           uint32_t width,
                           uint32_t height,
                           const VkBuffer& stagingBuffer) const;

    void transitionImageToShaderReadable(const VkCommandPool& commandPool) const;

    void cleanupStagingBuffer(VkBuffer& stagingBuffer,
                              VkDeviceMemory& stagingBufferMemory) const;

    void createImageView() override;
  };

} // vke

#endif //VULKANPROJECT_TEXTUREGLYPH_H