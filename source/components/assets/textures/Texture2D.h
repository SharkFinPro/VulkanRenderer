#ifndef VKE_TEXTURE2D_H
#define VKE_TEXTURE2D_H

#include "Texture.h"

namespace vke {

  class Texture2D final : public Texture {
  public:
    explicit Texture2D(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       vk::CommandPool commandPool,
                       const char* path,
                       vk::SamplerAddressMode samplerAddressMode);

  private:
    void createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            vk::CommandPool commandPool,
                            const char* path);

    void createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice) override;
  };

} // namespace vke

#endif //VKE_TEXTURE2D_H
