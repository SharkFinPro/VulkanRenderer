#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "Texture.h"

namespace vke {

class Texture2D final : public Texture {
public:
  explicit Texture2D(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const VkCommandPool& commandPool,
                     const char* path,
                     VkSamplerAddressMode samplerAddressMode);

private:
  void createTextureImage(const VkCommandPool& commandPool, const char* path);

  void createImageView() override;
};

} // namespace vke

#endif //TEXTURE2D_H
