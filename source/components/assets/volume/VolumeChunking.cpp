#include "VolumeChunking.h"
#include <glm/vector_relational.hpp>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace vke {

  VolumeChunkGrid::VolumeChunkGrid(const VolumeRegion& bounds,
                                   const uint32_t chunkCells)
    : VolumeChunkGrid(bounds, bounds, chunkCells)
  {}

  VolumeChunkGrid::VolumeChunkGrid(const VolumeRegion& bounds,
                                   const VolumeRegion& sampleBounds,
                                   const uint32_t chunkCells)
    : m_bounds(bounds),
      m_sampleBounds(sampleBounds),
      m_chunkCells(chunkCells)
  {
    if (chunkCells < 1 || chunkCells > 64)
    {
      throw std::invalid_argument("Volume chunk size must be between 1 and 64 cells.");
    }

    if (bounds.max.x < bounds.min.x || bounds.max.y < bounds.min.y || bounds.max.z < bounds.min.z)
    {
      throw std::invalid_argument("Volume bounds must have max >= min on every axis.");
    }

    if (glm::any(glm::greaterThan(sampleBounds.min, bounds.min)) || glm::any(glm::lessThan(sampleBounds.max, bounds.max)))
    {
      throw std::invalid_argument("Volume sample bounds must contain the bounds.");
    }

    m_cells = bounds.max - bounds.min - 1;

    if (m_cells.x < 1 || m_cells.y < 1 || m_cells.z < 1)
    {
      return;
    }

    const auto size = static_cast<int64_t>(chunkCells);
    uint64_t total = 1;

    for (int axis = 0; axis < 3; ++axis)
    {
      m_counts[axis] = static_cast<uint32_t>((m_cells[axis] + size - 1) / size);
      total *= m_counts[axis];
    }

    if (total > std::numeric_limits<uint32_t>::max())
    {
      throw std::invalid_argument("Volume has too many chunks.");
    }

    m_chunkCount = static_cast<uint32_t>(total);
  }

  const VolumeRegion& VolumeChunkGrid::getBounds() const
  {
    return m_bounds;
  }

  const VolumeRegion& VolumeChunkGrid::getSampleBounds() const
  {
    return m_sampleBounds;
  }

  uint32_t VolumeChunkGrid::getChunkCells() const
  {
    return m_chunkCells;
  }

  uint32_t VolumeChunkGrid::chunkCount() const
  {
    return m_chunkCount;
  }

  glm::uvec3 VolumeChunkGrid::chunkCounts() const
  {
    return m_counts;
  }

  CellRange VolumeChunkGrid::cellRange(const uint32_t chunk) const
  {
    const glm::ivec3 coordinates = chunkCoordinates(chunk);
    const glm::ivec3 first = m_bounds.min + coordinates * static_cast<int>(m_chunkCells);

    return { first, glm::min(first + static_cast<int>(m_chunkCells), m_bounds.max - 1) };
  }

  VolumeRegion VolumeChunkGrid::neededSamples(const uint32_t chunk) const
  {
    const CellRange cells = cellRange(chunk);

    return { cells.min - 1, cells.max + 2 };
  }

  VolumeRegion VolumeChunkGrid::fetchRegion(const uint32_t chunk) const
  {
    return intersect(neededSamples(chunk), m_sampleBounds);
  }

  AABB VolumeChunkGrid::localBounds(const uint32_t chunk,
                                    const float cellSize) const
  {
    const CellRange cells = cellRange(chunk);

    return { glm::vec3(cells.min) * cellSize, glm::vec3(cells.max) * cellSize };
  }

  void VolumeChunkGrid::touchedChunks(const VolumeRegion& region,
                                      std::vector<uint32_t>& out) const
  {
    out.clear();

    // No chunk needs a sample more than one past the bounds, and chunkRange maps only those correctly.
    const VolumeRegion reachable = intersect(m_sampleBounds, { m_bounds.min - 1, m_bounds.max + 1 });
    const VolumeRegion clipped = intersect(region, reachable);

    if (m_chunkCount == 0 || isEmpty(clipped))
    {
      return;
    }

    glm::ivec3 first { 0 };
    glm::ivec3 last { 0 };

    for (int axis = 0; axis < 3; ++axis)
    {
      chunkRange(axis, clipped.min[axis] - m_bounds.min[axis], clipped.max[axis] - m_bounds.min[axis], first[axis], last[axis]);
    }

    for (int z = first.z; z <= last.z; ++z)
    {
      for (int y = first.y; y <= last.y; ++y)
      {
        for (int x = first.x; x <= last.x; ++x)
        {
          out.push_back(static_cast<uint32_t>(x) + m_counts.x * (static_cast<uint32_t>(y) + m_counts.y * static_cast<uint32_t>(z)));
        }
      }
    }
  }

  glm::ivec3 VolumeChunkGrid::chunkCoordinates(const uint32_t chunk) const
  {
    return {
      static_cast<int>(chunk % m_counts.x),
      static_cast<int>((chunk / m_counts.x) % m_counts.y),
      static_cast<int>(chunk / (m_counts.x * m_counts.y))
    };
  }

  void VolumeChunkGrid::chunkRange(const int axis,
                                   const int low,
                                   const int high,
                                   int& first,
                                   int& last) const
  {
    const auto size = static_cast<int>(m_chunkCells);
    const auto count = static_cast<int>(m_counts[axis]);

    // Chunk c needs the samples [c * size - 1, min((c + 1) * size, cells) + 2). It starts before high when
    // c * size <= high, and ends after low when (c + 1) * size + 2 > low (the last chunk always does, since low is at
    // most cells + 1 there). Both hold for low = -1 and high = cells + 2, the furthest the caller reaches.
    last = std::min(count - 1, high / size);

    const int minimumEnd = low - 1;
    first = minimumEnd <= 0 ? 0 : std::max(0, (minimumEnd + size - 1) / size - 1);
  }

} // namespace vke
