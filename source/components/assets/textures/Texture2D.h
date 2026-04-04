#ifndef VKE_TEXTURE2D_H
#define VKE_TEXTURE2D_H

#include "Texture.h"

namespace vke {

  class Texture2D final : public Texture {
  public:
    explicit Texture2D(std::shared_ptr<LogicalDevice> logicalDevice,
                       const vk::raii::CommandPool& commandPool,
                       const char* path,
                       vk::SamplerAddressMode samplerAddressMode);

  private:
    void createTextureImage(const vk::raii::CommandPool& commandPool,
                            const char* path);

    void createImageView() override;
  };

} // namespace vke

#endif //VKE_TEXTURE2D_H
