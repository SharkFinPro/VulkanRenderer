#ifndef VKE_VOLUMEVERTEX_H
#define VKE_VOLUMEVERTEX_H

#include <glm/vec3.hpp>
#include <array>
#include <cstdint>

namespace vke {

  // The vertex a volume surface's meshes are drawn with. The materials and weights are four-way blend inputs, of which
  // only the first material (weight 255) is used until surfaces blend materials.
  struct VolumeVertex {
    glm::vec3 position;
    glm::vec3 normal;
    std::array<uint8_t, 4> materials;
    std::array<uint8_t, 4> weights;
  };

  static_assert(sizeof(VolumeVertex) == 32);

} // namespace vke

#endif //VKE_VOLUMEVERTEX_H
