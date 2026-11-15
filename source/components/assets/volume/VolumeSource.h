#ifndef VKE_VOLUMESOURCE_H
#define VKE_VOLUMESOURCE_H

#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <cstddef>
#include <cstdint>
#include <span>

namespace vke {

  // A box of integer sample coordinates; max is exclusive.
  struct VolumeRegion {
    glm::ivec3 min { 0 };
    glm::ivec3 max { 0 };
  };

  [[nodiscard]] inline bool isEmpty(const VolumeRegion& region)
  {
    return region.max.x <= region.min.x || region.max.y <= region.min.y || region.max.z <= region.min.z;
  }

  // Samples along each axis; zero on every axis of an empty region.
  [[nodiscard]] inline glm::ivec3 extent(const VolumeRegion& region)
  {
    return glm::max(region.max - region.min, glm::ivec3(0));
  }

  [[nodiscard]] inline size_t sampleCount(const VolumeRegion& region)
  {
    const glm::ivec3 size = extent(region);

    return static_cast<size_t>(size.x) * static_cast<size_t>(size.y) * static_cast<size_t>(size.z);
  }

  // The overlap; empty (see isEmpty) when the regions do not touch.
  [[nodiscard]] inline VolumeRegion intersect(const VolumeRegion& a,
                                              const VolumeRegion& b)
  {
    return { glm::max(a.min, b.min), glm::min(a.max, b.max) };
  }

  // Where the application's field lives. The engine pulls samples from it and keeps no copy.
  //
  // Contract:
  //  - A sample is a distance to the surface in cells (negative inside) and a material index. The surface is the zero
  //    set of the distance.
  //  - The engine requests only regions inside bounds(), which is read once, at creation. lod is always 0 for now.
  //  - fill() writes x-fastest: index = x + sx * (y + sy * z) relative to region.min, where s is the region's size.
  //  - Threads: fill() runs on engine worker threads, several at once and concurrently with the frame thread. An edit
  //    and the VolumeSurface::invalidate call that reports it must appear atomic to fill(): for instance, hold a
  //    std::shared_mutex exclusively across both and shared inside fill(). An edit that fill() can observe before its
  //    invalidate() may briefly show a crack. All VolumeSurface and AssetManager calls are frame-thread only.
  class VolumeSource {
  public:
    virtual ~VolumeSource() = default;

    virtual void fill(const VolumeRegion& region,
                      uint32_t lod,
                      std::span<float> distance,
                      std::span<uint8_t> material) const = 0;

    [[nodiscard]] virtual VolumeRegion bounds() const = 0;
  };

  struct VolumeSurfaceDescription {
    // Size of one cell in the surface's local space.
    float cellSize = 1.0f;
    // Cells per chunk edge, 1 to 256. A chunk is extracted and uploaded as a unit.
    uint32_t chunkCells = 32;
  };

} // namespace vke

#endif //VKE_VOLUMESOURCE_H
