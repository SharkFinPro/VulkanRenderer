#ifndef CUBEMAPTEXTURE_H
#define CUBEMAPTEXTURE_H

#include "Texture.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <string>

class LogicalDevice;

class TextureCubemap final : public Texture {
public:
  TextureCubemap(const std::shared_ptr<LogicalDevice> &logicalDevice,
                 const VkCommandPool& commandPool,
                 const std::array<std::string, 6>& paths);

private:
  void createTextureImage(const VkCommandPool& commandPool, const std::array<std::string, 6>& paths);

  void copyBufferToImage(const VkCommandPool& commandPool,
                         VkBuffer stagingBuffer,
                         VkDeviceSize imageSize,
                         uint32_t textureWidth,
                         uint32_t textureHeight) const;

  void createImageView() override;
};



#endif //CUBEMAPTEXTURE_H
