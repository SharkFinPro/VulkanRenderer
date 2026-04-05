#ifndef VKE_CUBEMAPTEXTURE_H
#define VKE_CUBEMAPTEXTURE_H

#include "Texture.h"
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <memory>
#include <string>

namespace vke {

  class LogicalDevice;

  class TextureCubemap final : public Texture {
  public:
    TextureCubemap(std::shared_ptr<LogicalDevice> logicalDevice,
                   const vk::raii::CommandPool& commandPool,
                   const std::array<std::string, 6>& paths);

  private:
    void createTextureImage(const vk::raii::CommandPool& commandPool,
                            const std::array<std::string, 6>& paths);

    void createImage(const vk::raii::Buffer& stagingBuffer,
                     const vk::raii::CommandPool& commandPool,
                     vk::DeviceSize imageSize,
                     uint32_t texWidth,
                     uint32_t texHeight);

    void copyBufferToImage(const vk::raii::CommandPool& commandPool,
                           const vk::raii::Buffer& stagingBuffer,
                           vk::DeviceSize imageSize,
                           uint32_t textureWidth,
                           uint32_t textureHeight) const;

    void createImageView() override;
  };

} // namespace vke

#endif //VKE_CUBEMAPTEXTURE_H
