#ifndef VKE_TEXTURE3D_H
#define VKE_TEXTURE3D_H

#include "Texture.h"

namespace vke {

  class Texture3D final : public Texture {
  public:
    Texture3D(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const VkCommandPool& commandPool,
              const char* path,
              VkSamplerAddressMode samplerAddressMode);

  private:
    void createTextureImage(const VkCommandPool& commandPool, const char* path);

    void createImageView() override;
  };

} // namespace vke

#endif //VKE_TEXTURE3D_H
