#ifndef VKE_TEXTURE3D_H
#define VKE_TEXTURE3D_H

#include "Texture.h"

namespace vke {

  class Texture3D final : public Texture {
  public:
    Texture3D(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const vk::raii::CommandPool& commandPool,
              const char* path,
              vk::SamplerAddressMode samplerAddressMode);

  private:
    void createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            const vk::raii::CommandPool& commandPool,
                            const char* path);

    void createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice) override;
  };

} // namespace vke

#endif //VKE_TEXTURE3D_H
