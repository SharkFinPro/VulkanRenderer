#ifndef VKE_SURFACEEXTRACTOR_H
#define VKE_SURFACEEXTRACTOR_H

#include "VolumeSource.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace vke {

  struct SurfaceVertex {
    glm::vec3 position;
    glm::vec3 normal;
    uint32_t material;
  };

  static_assert(sizeof(SurfaceVertex) == 28);

  struct SurfaceMesh {
    std::vector<SurfaceVertex> vertices;
    std::vector<uint32_t> indices;

    void clear()
    {
      vertices.clear();
      indices.clear();
    }
  };

  // Cells between samples, max exclusive: cell c has the samples c and c + 1 on each axis as its corners.
  struct CellRange {
    glm::ivec3 min { 0 };
    glm::ivec3 max { 0 };
  };

  // A read-only view of fetched samples. Reads clamp to the view's region, which is how reads past the volume's sample
  // bounds are answered.
  struct VolumeSamples {
    VolumeRegion region;
    std::span<const float> distance;
    std::span<const uint8_t> material;

    [[nodiscard]] size_t index(glm::ivec3 position) const
    {
      position = glm::clamp(position, region.min, region.max - 1);

      const glm::ivec3 local = position - region.min;
      const glm::ivec3 size = extent(region);

      return static_cast<size_t>(local.x) +
             static_cast<size_t>(size.x) * (static_cast<size_t>(local.y) + static_cast<size_t>(size.y) * static_cast<size_t>(local.z));
    }

    [[nodiscard]] float distanceAt(const glm::ivec3 position) const
    {
      return distance[index(position)];
    }

    [[nodiscard]] uint8_t materialAt(const glm::ivec3 position) const
    {
      return material[index(position)];
    }
  };

  // Turns samples into triangles. Implementations must make neighboring chunks agree on every shared vertex, since
  // chunks are extracted independently.
  class SurfaceExtractor {
  public:
    virtual ~SurfaceExtractor() = default;

    // Thread-safe: called from several workers at once. Clears out first. The samples must cover what the cells need
    // (see VolumeChunkGrid::neededSamples), clipped to the volume's sample bounds.
    virtual void extract(const VolumeSamples& samples,
                         const CellRange& cells,
                         float cellSize,
                         SurfaceMesh& out) const = 0;
  };

} // namespace vke

#endif //VKE_SURFACEEXTRACTOR_H
