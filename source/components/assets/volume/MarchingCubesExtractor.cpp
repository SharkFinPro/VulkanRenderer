#include "MarchingCubesExtractor.h"
#include "MarchingCubesTables.h"
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
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

    // The vertex on the edge from lower to lower + axis. It depends on nothing but the samples around that edge. The
    // caller passes samples clipped to the volume's sample bounds, so clamping to them equals clamping to the sample bounds, and two
    // chunks that share the edge read the same values.
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
        .material = samples.materialAt(d0 < 0.0f ? lower : upper)
      };
    }

    // Walks the cells of one call, creating each crossing edge's vertex once.
    class CellMesher {
    public:
      CellMesher(const VolumeSamples& samples,
                 const CellRange& cells,
                 const float cellSize,
                 SurfaceMesh& out,
                 std::vector<uint32_t>& edgeVertices)
        : m_samples(samples),
          m_origin(cells.min),
          m_size(cells.max - cells.min),
          m_cellSize(cellSize),
          m_out(out),
          m_edgeVertices(edgeVertices)
      {
        const glm::ivec3 points = m_size + 1;

        m_edgeVertices.assign(static_cast<size_t>(points.x) * static_cast<size_t>(points.y) * static_cast<size_t>(points.z) * 3, s_noVertex);
      }

      void run()
      {
        for (int z = 0; z < m_size.z; ++z)
        {
          for (int y = 0; y < m_size.y; ++y)
          {
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

      [[nodiscard]] unsigned cubeIndex(const glm::ivec3 cell) const
      {
        unsigned index = 0;

        for (unsigned corner = 0; corner < 8; ++corner)
        {
          const auto& offset = marchingCubes::cornerOffsets[corner];
          const glm::ivec3 position = m_origin + cell + glm::ivec3(offset[0], offset[1], offset[2]);

          if (m_samples.distanceAt(position) < 0.0f)
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

        for (int i = 0; i < 15 && triangles[i] >= 0; ++i)
        {
          m_out.indices.push_back(edgeVertex(cell, triangles[i]));
        }
      }

      [[nodiscard]] uint32_t edgeVertex(const glm::ivec3 cell,
                                        const int edge)
      {
        const marchingCubes::EdgeId& id = marchingCubes::edges[edge];
        const glm::ivec3 lower = cell + glm::ivec3(id.base[0], id.base[1], id.base[2]);
        const glm::ivec3 points = m_size + 1;

        const size_t slot = (static_cast<size_t>(lower.x) +
                             static_cast<size_t>(points.x) * (static_cast<size_t>(lower.y) + static_cast<size_t>(points.y) * static_cast<size_t>(lower.z))) * 3 +
                            static_cast<size_t>(id.axis);

        if (m_edgeVertices[slot] == s_noVertex)
        {
          m_edgeVertices[slot] = static_cast<uint32_t>(m_out.vertices.size());
          m_out.vertices.push_back(makeVertex(m_samples, m_origin + lower, id.axis, m_cellSize));
        }

        return m_edgeVertices[slot];
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

    CellMesher(samples, cells, cellSize, out, edgeVertices).run();
  }

} // namespace vke
