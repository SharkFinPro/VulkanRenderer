#ifndef TEXTURE3D_H
#define TEXTURE3D_H

#include "Texture.h"

class Texture3D final : public Texture {
public:
  explicit Texture3D(const std::shared_ptr<LogicalDevice>& logicalDevice);

protected:
  void createTextureImage(const VkCommandPool& commandPool, const char* path) override;
};



#endif //TEXTURE3D_H
