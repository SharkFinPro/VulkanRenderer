#ifndef VKE_VOLUMEMATERIAL_H
#define VKE_VOLUMEMATERIAL_H

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

namespace vke {

  class Texture2D;

  // One entry of a volume surface's palette, chosen per vertex by material index (see VolumeSurface::setPalette).
  struct VolumeMaterial {
    // Null: white
    std::shared_ptr<Texture2D> albedo;
    // Null: white
    std::shared_ptr<Texture2D> specular;
    // How many times the textures repeat per world unit
    float textureScale = 1.0f;
  };

  inline constexpr size_t s_maxVolumeMaterials = 16;

  // Throws std::invalid_argument on more than s_maxVolumeMaterials entries or a texture scale that is not finite and
  // positive.
  inline void validateVolumePalette(const std::span<const VolumeMaterial> palette)
  {
    if (palette.size() > s_maxVolumeMaterials)
    {
      throw std::invalid_argument("a volume surface palette holds at most " + std::to_string(s_maxVolumeMaterials) +
                                  " materials, got " + std::to_string(palette.size()));
    }

    for (size_t i = 0; i < palette.size(); i++)
    {
      if (!std::isfinite(palette[i].textureScale) || palette[i].textureScale <= 0.0f)
      {
        throw std::invalid_argument("volume material " + std::to_string(i) +
                                    " needs a texture scale that is finite and positive");
      }
    }
  }

  // The default palette's albedo colors
  inline constexpr std::array<glm::vec3, 8> s_defaultVolumeColors {
    glm::vec3(0.60f, 0.60f, 0.60f),
    glm::vec3(0.55f, 0.75f, 0.40f),
    glm::vec3(0.70f, 0.55f, 0.35f),
    glm::vec3(0.55f, 0.60f, 0.80f),
    glm::vec3(0.85f, 0.80f, 0.55f),
    glm::vec3(0.80f, 0.50f, 0.50f),
    glm::vec3(0.50f, 0.75f, 0.75f),
    glm::vec3(0.75f, 0.60f, 0.80f)
  };

  // Mirrored by common/VolumeMaterial.glsl
  inline constexpr float s_triplanarSharpness = 4.0f;
  inline constexpr float s_triplanarMinWeight = 0.02f;

  // The weights of the x, y and z projections for a unit normal: the normal's components raised to the sharpness and
  // normalized, with weights below the minimum dropped and the rest normalized again.
  [[nodiscard]] inline glm::vec3 triplanarWeights(const glm::vec3 normal)
  {
    glm::vec3 weights = glm::pow(glm::abs(normal), glm::vec3(s_triplanarSharpness));
    weights /= weights.x + weights.y + weights.z;

    for (int i = 0; i < 3; i++)
    {
      if (weights[i] < s_triplanarMinWeight)
      {
        weights[i] = 0.0f;
      }
    }

    return weights / (weights.x + weights.y + weights.z);
  }

} // namespace vke

#endif //VKE_VOLUMEMATERIAL_H
