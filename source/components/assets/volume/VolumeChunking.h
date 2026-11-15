#ifndef VKE_VOLUMECHUNKING_H
#define VKE_VOLUMECHUNKING_H

#include "SurfaceExtractor.h"
#include "VolumeSource.h"
#include "../../../utilities/Bounds.h"
#include <glm/vec3.hpp>
#include <cstdint>
#include <vector>

namespace vke {

  // A regular grid of chunks over a volume's bounds. Cells sit between samples, so an axis with n samples has n - 1
  // cells, and chunkCells cells make a chunk (the last one on an axis may be smaller). Chunks are indexed x-fastest.
  //
  // A chunk's cells [start, end) need the samples [start - 1, end + 2): the endpoints of every edge a cell crosses and
  // the neighbors a central difference reads at them. A sample edit therefore touches the chunks around it too.
  class VolumeChunkGrid {
  public:
    // Throws std::invalid_argument unless chunkCells is in [1, 256] and max >= min on every axis of the bounds.
    VolumeChunkGrid(const VolumeRegion& bounds,
                    uint32_t chunkCells);

    [[nodiscard]] const VolumeRegion& getBounds() const;

    [[nodiscard]] uint32_t getChunkCells() const;

    // Zero when any axis has fewer than two samples.
    [[nodiscard]] uint32_t chunkCount() const;

    [[nodiscard]] glm::uvec3 chunkCounts() const;

    [[nodiscard]] CellRange cellRange(uint32_t chunk) const;

    // The samples the chunk's cells read, which can reach past the bounds.
    [[nodiscard]] VolumeRegion neededSamples(uint32_t chunk) const;

    // neededSamples clipped to the bounds: what to fetch, and what the extractor clamps reads to.
    [[nodiscard]] VolumeRegion fetchRegion(uint32_t chunk) const;

    // Chunk-local bounds in surface local space: the cell range times cellSize.
    [[nodiscard]] AABB localBounds(uint32_t chunk,
                                   float cellSize) const;

    // Every chunk whose needed samples intersect the region, ascending. Clears out first. Samples outside the bounds
    // are ignored.
    void touchedChunks(const VolumeRegion& region,
                       std::vector<uint32_t>& out) const;

  private:
    VolumeRegion m_bounds;
    uint32_t m_chunkCells;
    glm::ivec3 m_cells { 0 };
    glm::uvec3 m_counts { 0 };
    uint32_t m_chunkCount = 0;

    [[nodiscard]] glm::ivec3 chunkCoordinates(uint32_t chunk) const;

    // The first and last chunk along one axis whose needed samples reach [low, high), in samples relative to the
    // bounds' minimum.
    void chunkRange(int axis,
                    int low,
                    int high,
                    int& first,
                    int& last) const;
  };

} // namespace vke

#endif //VKE_VOLUMECHUNKING_H
