#ifndef VKE_VOLUMEVERTEX_H
#define VKE_VOLUMEVERTEX_H

#include <glm/vec3.hpp>
#include <array>
#include <cstddef>
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

  // The ray tracing closest hit shader mirrors this layout
  static_assert(sizeof(VolumeVertex) == 32);
  static_assert(offsetof(VolumeVertex, normal) == 12);
  static_assert(offsetof(VolumeVertex, materials) == 24);
  static_assert(offsetof(VolumeVertex, weights) == 28);

} // namespace vke

#endif //VKE_VOLUMEVERTEX_H
