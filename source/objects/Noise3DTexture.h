#ifndef NOISE3DTEXTURE_H
#define NOISE3DTEXTURE_H

#include "Texture.h"

class Noise3DTexture final : public Texture {
  void createTextureImage(const VkCommandPool& commandPool, const char* path) override;
};



#endif //NOISE3DTEXTURE_H
