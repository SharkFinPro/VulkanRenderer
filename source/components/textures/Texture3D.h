#ifndef TEXTURE3D_H
#define TEXTURE3D_H

#include "Texture.h"

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



#endif //TEXTURE3D_H
