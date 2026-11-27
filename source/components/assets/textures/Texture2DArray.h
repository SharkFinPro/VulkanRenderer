#ifndef VKE_TEXTURE2DARRAY_H
#define VKE_TEXTURE2DARRAY_H

#include "Texture.h"
#include <glm/vec4.hpp>
#include <cstdint>
#include <span>

namespace vke {

  class Texture2D;

  // One layer of a Texture2DArray: a copy of a texture, or a solid color.
  struct Texture2DArrayLayer {
    // Null: the layer is filled with `fill`
    std::shared_ptr<Texture2D> source;
    glm::vec4 fill { 1.0f };
  };

  // A 2D array texture with a full mip chain, built synchronously from existing textures and solid colors. Every layer
  // has the extent of the largest source (1x1 with none), so smaller sources are scaled up. The sources are copied, so
  // the array does not keep them alive.
  class Texture2DArray final : public Texture {
  public:
    // Throws std::invalid_argument on zero layers.
    Texture2DArray(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   vk::CommandPool commandPool,
                   std::span<const Texture2DArrayLayer> layers,
                   vk::SamplerAddressMode samplerAddressMode);

    [[nodiscard]] uint32_t getLayerCount() const;

  private:
    uint32_t m_layerCount = 0;

    void createTextureImage(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            vk::CommandPool commandPool,
                            std::span<const Texture2DArrayLayer> layers);

    void createImageView(const std::shared_ptr<LogicalDevice>& logicalDevice) override;
  };

} // namespace vke

#endif //VKE_TEXTURE2DARRAY_H
