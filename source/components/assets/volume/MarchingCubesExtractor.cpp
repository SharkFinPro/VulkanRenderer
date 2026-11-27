#include "MarchingCubesExtractor.h"
#include "MarchingCubesTables.h"
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace vke {

  namespace {

    constexpr uint32_t s_noVertex = std::numeric_limits<uint32_t>::max();

    [[nodiscard]] glm::ivec3 axisStep(const int axis)
    {
      glm::ivec3 step { 0 };
      step[axis] = 1;

      return step;
    }

    // Central difference at a sample; the field increases outward, so this points out of the solid.
    [[nodiscard]] glm::vec3 gradient(const VolumeSamples& samples,
                                     const glm::ivec3 position)
    {
      glm::vec3 result;

      for (int axis = 0; axis < 3; ++axis)
      {
        const glm::ivec3 step = axisStep(axis);

        result[axis] = 0.5f * (samples.distanceAt(position + step) - samples.distanceAt(position - step));
      }

      return result;
    }

    // The material of the edge's inside sample
    [[nodiscard]] uint8_t edgeMaterial(const VolumeSamples& samples,
                                       const glm::ivec3 lower,
                                       const int axis)
    {
      return samples.materialAt(samples.distanceAt(lower) < 0.0f ? lower : lower + axisStep(axis));
    }

    // The vertex on the edge from lower to lower + axis, with its own material at full weight. It depends on nothing but
    // the samples around that edge. The caller passes samples clipped to the volume's sample bounds, so a read past the
    // view clamps exactly as a read past the sample bounds would, and two chunks that share the edge read the same values.
    [[nodiscard]] SurfaceVertex makeVertex(const VolumeSamples& samples,
                                           const glm::ivec3 lower,
                                           const int axis,
                                           const float cellSize)
    {
      const glm::ivec3 upper = lower + axisStep(axis);
      const float d0 = samples.distanceAt(lower);
      const float d1 = samples.distanceAt(upper);

      float t = d0 / (d0 - d1);

      if (!(t >= 0.0f && t <= 1.0f))
      {
        t = 0.5f;
      }

      const glm::vec3 normal = glm::mix(gradient(samples, lower), gradient(samples, upper), t);
      const float length = glm::length(normal);

      return {
        .position = (glm::vec3(lower) + t * glm::vec3(upper - lower)) * cellSize,
        .normal = length > 1e-12f ? normal / length : glm::vec3(0.0f, 1.0f, 0.0f),
        .materials = { edgeMaterial(samples, lower, axis), 0, 0, 0 },
        .weights = { 255, 0, 0, 0 }
      };
    }

    // The distinct materials of a triangle's corners, ascending
    struct MaterialSet {
      std::array<uint8_t, 3> materials {};
      uint32_t count = 0;

      [[nodiscard]] uint32_t packed() const
      {
        return static_cast<uint32_t>(materials[0]) | (static_cast<uint32_t>(materials[1]) << 8) |
               (static_cast<uint32_t>(materials[2]) << 16) | (count << 24);
      }
    };

    [[nodiscard]] MaterialSet materialSet(std::array<uint8_t, 3> corners)
    {
      std::ranges::sort(corners);

      MaterialSet set;

      for (const uint8_t material : corners)
      {
        if (set.count == 0 || set.materials[set.count - 1] != material)
        {
          set.materials[set.count++] = material;
        }
      }

      return set;
    }

    // Walks the cells of one call, creating each crossing edge's vertex once per material set it is used with.
    class CellMesher {
    public:
      CellMesher(const VolumeSamples& samples,
                 const CellRange& cells,
                 const float cellSize,
                 SurfaceMesh& out,
                 std::vector<uint32_t>& edgeVertices,
                 std::unordered_map<uint64_t, uint32_t>& blendedVertices,
                 std::vector<uint8_t>& inside)
        : m_samples(samples),
          m_origin(cells.min),
          m_size(cells.max - cells.min),
          m_cellSize(cellSize),
          m_out(out),
          m_edgeVertices(edgeVertices),
          m_blendedVertices(blendedVertices),
          m_inside(inside)
      {
        const glm::ivec3 points = m_size + 1;

        m_blendedVertices.clear();
        m_rowSigns.assign(static_cast<size_t>(points.y) * static_cast<size_t>(points.z), 0);

        // Each corner's sign is read once here instead of once per cell that shares it (eight times).
        m_inside.resize(static_cast<size_t>(points.x) * static_cast<size_t>(points.y) * static_cast<size_t>(points.z));

        size_t next = 0;

        const VolumeRegion& region = m_samples.region;
        const glm::ivec3 regionSize = extent(region);
        const glm::ivec3 last = m_origin + points;
        const bool covered = m_origin.x >= region.min.x && m_origin.y >= region.min.y && m_origin.z >= region.min.z &&
                             last.x <= region.max.x && last.y <= region.max.y && last.z <= region.max.z;

        for (int z = 0; z < points.z; ++z)
        {
          for (int y = 0; y < points.y; ++y)
          {
            if (covered)
            {
              // No read needs clamping, so the row is read straight from the samples
              const size_t row = static_cast<size_t>(m_origin.x - region.min.x) +
                                 static_cast<size_t>(regionSize.x) *
                                   (static_cast<size_t>(m_origin.y + y - region.min.y) +
                                    static_cast<size_t>(regionSize.y) * static_cast<size_t>(m_origin.z + z - region.min.z));
              const float* distances = m_samples.distance.data() + row;

              uint8_t seen = 0;

              for (int x = 0; x < points.x; ++x)
              {
                const uint8_t sign = distances[x] < 0.0f ? 1 : 0;
                seen |= static_cast<uint8_t>(1u << sign);
                m_inside[next++] = sign;
              }

              m_rowSigns[static_cast<size_t>(z) * static_cast<size_t>(points.y) + static_cast<size_t>(y)] = seen;
              continue;
            }

            uint8_t seen = 0;

            for (int x = 0; x < points.x; ++x)
            {
              const uint8_t sign = m_samples.distanceAt(m_origin + glm::ivec3(x, y, z)) < 0.0f ? 1 : 0;
              seen |= static_cast<uint8_t>(1u << sign);
              m_inside[next++] = sign;
            }

            m_rowSigns[static_cast<size_t>(z) * static_cast<size_t>(points.y) + static_cast<size_t>(y)] = seen;
          }
        }
      }

      void run()
      {
        const glm::ivec3 points = m_size + 1;
        const auto rowSigns = [&](const int y, const int z) {
          return m_rowSigns[static_cast<size_t>(z) * static_cast<size_t>(points.y) + static_cast<size_t>(y)];
        };

        // A bit per sign seen in a row of corners: 1 outside, 2 inside. A cell row whose four corner rows hold one sign
        // only has no surface, so most rows of a chunk that is mostly air or solid are skipped unread.
        uint8_t everything = 0;

        for (const uint8_t signs : m_rowSigns)
        {
          everything |= signs;
        }

        if (everything != 3)
        {
          return;
        }

        m_edgeVertices.assign(static_cast<size_t>(points.x) * static_cast<size_t>(points.y) * static_cast<size_t>(points.z) * 3, s_noVertex);

        for (int z = 0; z < m_size.z; ++z)
        {
          for (int y = 0; y < m_size.y; ++y)
          {
            const uint8_t around = rowSigns(y, z) | rowSigns(y + 1, z) | rowSigns(y, z + 1) | rowSigns(y + 1, z + 1);

            if (around != 3)
            {
              continue;
            }

            for (int x = 0; x < m_size.x; ++x)
            {
              meshCell({ x, y, z });
            }
          }
        }
      }

    private:
      const VolumeSamples& m_samples;
      glm::ivec3 m_origin;
      glm::ivec3 m_size;
      float m_cellSize;
      SurfaceMesh& m_out;
      std::vector<uint32_t>& m_edgeVertices;
      std::unordered_map<uint64_t, uint32_t>& m_blendedVertices;
      std::vector<uint8_t>& m_inside;
      std::vector<uint8_t> m_rowSigns;

      [[nodiscard]] bool insideAt(const glm::ivec3 local) const
      {
        const glm::ivec3 points = m_size + 1;

        return m_inside[static_cast<size_t>(local.x) +
                        static_cast<size_t>(points.x) * (static_cast<size_t>(local.y) + static_cast<size_t>(points.y) * static_cast<size_t>(local.z))] != 0;
      }

      [[nodiscard]] unsigned cubeIndex(const glm::ivec3 cell) const
      {
        unsigned index = 0;

        for (unsigned corner = 0; corner < 8; ++corner)
        {
          const auto& offset = marchingCubes::cornerOffsets[corner];
          if (insideAt(cell + glm::ivec3(offset[0], offset[1], offset[2])))
          {
            index |= 1u << corner;
          }
        }

        return index;
      }

      void meshCell(const glm::ivec3 cell)
      {
        const unsigned index = cubeIndex(cell);

        if (index == 0 || index == 255)
        {
          return;
        }

        const int8_t* triangles = marchingCubes::triTable[index];

        for (int i = 0; i < 15 && triangles[i] >= 0; i += 3)
        {
          const MaterialSet set = materialSet({
            cornerMaterial(cell, triangles[i]),
            cornerMaterial(cell, triangles[i + 1]),
            cornerMaterial(cell, triangles[i + 2])
          });

          for (int k = 0; k < 3; ++k)
          {
            m_out.indices.push_back(set.count == 1 ? edgeVertex(cell, triangles[i + k])
                                                   : blendedVertex(cell, triangles[i + k], set));
          }
        }
      }

      [[nodiscard]] static glm::ivec3 edgeLower(const glm::ivec3 cell,
                                                const marchingCubes::EdgeId& id)
      {
        return cell + glm::ivec3(id.base[0], id.base[1], id.base[2]);
      }

      [[nodiscard]] size_t edgeSlot(const glm::ivec3 lower,
                                    const int axis) const
      {
        const glm::ivec3 points = m_size + 1;

        return (static_cast<size_t>(lower.x) +
                static_cast<size_t>(points.x) * (static_cast<size_t>(lower.y) + static_cast<size_t>(points.y) * static_cast<size_t>(lower.z))) * 3 +
               static_cast<size_t>(axis);
      }

      [[nodiscard]] uint8_t cornerMaterial(const glm::ivec3 cell,
                                           const int edge) const
      {
        const marchingCubes::EdgeId& id = marchingCubes::edges[edge];

        const glm::ivec3 lower = edgeLower(cell, id);

        return m_samples.materialAt(m_origin + (insideAt(lower) ? lower : lower + axisStep(id.axis)));
      }

      // The edge's vertex for triangles of one material
      [[nodiscard]] uint32_t edgeVertex(const glm::ivec3 cell,
                                        const int edge)
      {
        const marchingCubes::EdgeId& id = marchingCubes::edges[edge];
        const glm::ivec3 lower = edgeLower(cell, id);
        const size_t slot = edgeSlot(lower, id.axis);

        if (m_edgeVertices[slot] == s_noVertex)
        {
          m_edgeVertices[slot] = static_cast<uint32_t>(m_out.vertices.size());
          m_out.vertices.push_back(makeVertex(m_samples, m_origin + lower, id.axis, m_cellSize));
        }

        return m_edgeVertices[slot];
      }

      // The edge's vertex for triangles with these materials: it lists them all and weighs its own fully
      [[nodiscard]] uint32_t blendedVertex(const glm::ivec3 cell,
                                           const int edge,
                                           const MaterialSet& set)
      {
        const marchingCubes::EdgeId& id = marchingCubes::edges[edge];
        const glm::ivec3 lower = edgeLower(cell, id);
        const uint64_t key = (static_cast<uint64_t>(edgeSlot(lower, id.axis)) << 32) | set.packed();

        const auto [it, inserted] = m_blendedVertices.try_emplace(key, static_cast<uint32_t>(m_out.vertices.size()));

        if (inserted)
        {
          SurfaceVertex vertex = makeVertex(m_samples, m_origin + lower, id.axis, m_cellSize);
          const uint8_t own = vertex.materials[0];

          vertex.materials = { 0, 0, 0, 0 };
          vertex.weights = { 0, 0, 0, 0 };

          for (uint32_t i = 0; i < set.count; ++i)
          {
            vertex.materials[i] = set.materials[i];
            vertex.weights[i] = set.materials[i] == own ? 255 : 0;
          }

          m_out.vertices.push_back(vertex);
        }

        return it->second;
      }
    };

  } // namespace

  void MarchingCubesExtractor::extract(const VolumeSamples& samples,
                                       const CellRange& cells,
                                       const float cellSize,
                                       SurfaceMesh& out) const
  {
    out.clear();

    if (cells.max.x <= cells.min.x || cells.max.y <= cells.min.y || cells.max.z <= cells.min.z)
    {
      return;
    }

    // Reused by the calls of this thread, so a worker does not allocate per chunk
    thread_local std::vector<uint32_t> edgeVertices;
    thread_local std::unordered_map<uint64_t, uint32_t> blendedVertices;
    thread_local std::vector<uint8_t> inside;

    CellMesher(samples, cells, cellSize, out, edgeVertices, blendedVertices, inside).run();
  }

} // namespace vke
