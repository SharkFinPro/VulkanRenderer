#ifndef NOISE3DTEXTURE_H
#define NOISE3DTEXTURE_H

#include "Texture.h"

class Noise3DTexture final : public Texture {
public:
  Noise3DTexture(const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool);

private:
  void createTextureImage(const VkCommandPool& commandPool, const char* path) override;
};



#endif //NOISE3DTEXTURE_H
