// Headless checks of the Marching Cubes surface extractor and its chunk grid on analytic and random fields: closed and
// consistently wound meshes, vertices on the zero set, neighboring chunks agreeing bit for bit, determinism and edge
// clamping. No engine, no GPU: ctest runs this.
#include "source/components/assets/volume/MarchingCubesExtractor.h"
#include "source/components/assets/volume/VolumeChunking.h"
#include "source/components/assets/volume/VolumeSource.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const std::string& name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  using Field = std::function<float(glm::vec3)>;
  using MaterialField = std::function<uint8_t(glm::ivec3)>;

  // Varied enough that every field above meets several materials
  uint8_t stripedMaterial(const glm::ivec3 position)
  {
    return static_cast<uint8_t>((position.x / 7 + position.y / 5 + position.z / 3) % 5);
  }

  constexpr float s_cellSize = 0.5f;

  // A field sampled into memory over its sample bounds (the bounds unless given). Requests outside the sample bounds
  // are failures, as the engine never makes them.
  class GridSource final : public vke::VolumeSource {
  public:
    GridSource(const vke::VolumeRegion& bounds,
               const Field& field,
               const MaterialField& materialField = stripedMaterial)
      : GridSource(bounds, bounds, field, materialField)
    {}

    GridSource(const vke::VolumeRegion& bounds,
               const vke::VolumeRegion& sampleBounds,
               const Field& field,
               const MaterialField& materialField = stripedMaterial)
      : m_bounds(bounds),
        m_sampleBounds(sampleBounds),
        m_size(vke::extent(sampleBounds)),
        m_distance(vke::sampleCount(sampleBounds)),
        m_material(vke::sampleCount(sampleBounds))
    {
      for (int z = 0; z < m_size.z; ++z)
      {
        for (int y = 0; y < m_size.y; ++y)
        {
          for (int x = 0; x < m_size.x; ++x)
          {
            const glm::ivec3 position = sampleBounds.min + glm::ivec3(x, y, z);
            const size_t index = indexOf(position);

            m_distance[index] = field(glm::vec3(position));
            m_material[index] = materialField(position);
          }
        }
      }
    }

    void fill(const vke::VolumeRegion& region,
              uint32_t,
              std::span<float> distance,
              std::span<uint8_t> material) const override
    {
      const vke::VolumeRegion inside = vke::intersect(region, m_sampleBounds);
      check(inside.min == region.min && inside.max == region.max, "fill stays inside the sample bounds");
      check(distance.size() == vke::sampleCount(region) && material.size() == distance.size(), "fill spans match the region");

      if (inside.min != region.min || inside.max != region.max)
      {
        return;
      }

      const glm::ivec3 size = vke::extent(region);
      size_t out = 0;

      for (int z = 0; z < size.z; ++z)
      {
        for (int y = 0; y < size.y; ++y)
        {
          for (int x = 0; x < size.x; ++x, ++out)
          {
            const size_t index = indexOf(region.min + glm::ivec3(x, y, z));

            distance[out] = m_distance[index];
            material[out] = m_material[index];
          }
        }
      }
    }

    [[nodiscard]] vke::VolumeRegion bounds() const override
    {
      return m_bounds;
    }

    [[nodiscard]] vke::VolumeRegion sampleBounds() const override
    {
      return m_sampleBounds;
    }

    [[nodiscard]] float sampleDistance(const glm::ivec3 position) const
    {
      return m_distance[indexOf(position)];
    }

    [[nodiscard]] uint8_t sampleMaterial(const glm::ivec3 position) const
    {
      return m_material[indexOf(position)];
    }

  private:
    vke::VolumeRegion m_bounds;
    vke::VolumeRegion m_sampleBounds;
    glm::ivec3 m_size;
    std::vector<float> m_distance;
    std::vector<uint8_t> m_material;

    [[nodiscard]] size_t indexOf(const glm::ivec3 position) const
    {
      const glm::ivec3 local = position - m_sampleBounds.min;

      return static_cast<size_t>(local.x) + static_cast<size_t>(m_size.x) * (static_cast<size_t>(local.y) + static_cast<size_t>(m_size.y) * static_cast<size_t>(local.z));
    }
  };

  vke::SurfaceMesh extractChunk(const GridSource& source,
                                const vke::VolumeChunkGrid& grid,
                                const uint32_t chunk)
  {
    const vke::VolumeRegion region = grid.fetchRegion(chunk);

    std::vector<float> distance(vke::sampleCount(region));
    std::vector<uint8_t> material(vke::sampleCount(region));
    source.fill(region, 0, distance, material);

    vke::SurfaceMesh mesh;
    vke::MarchingCubesExtractor().extract({ region, distance, material }, grid.cellRange(chunk), s_cellSize, mesh);

    return mesh;
  }

  // Fields in cells

  Field sphereField()
  {
    return [](const glm::vec3 p) {
      return glm::length(p - glm::vec3(16.3f, 15.7f, 16.1f)) - 9.3f;
    };
  }

  Field planeField()
  {
    return [](const glm::vec3 p) {
      return glm::dot(glm::normalize(glm::vec3(1.0f, 2.0f, 0.5f)), p) - 17.3f;
    };
  }

  Field torusField()
  {
    return [](const glm::vec3 p) {
      const glm::vec2 q(glm::length(glm::vec2(p.x - 16.2f, p.z - 15.8f)) - 8.1f, p.y - 16.4f);

      return glm::length(q) - 3.6f;
    };
  }

  // A slab with a sphere carved out of it, reaching the volume's sides. The carve is a smooth maximum, since a sharp
  // crease has no meaningful gradient for a vertex normal.
  Field slabField()
  {
    return [](const glm::vec3 p) {
      const float slab = std::abs(p.y - 15.6f) - 6.2f;
      const float hole = -(glm::length(p - glm::vec3(15.4f, 17.5f, 16.2f)) - 7.7f);

      constexpr float blend = 3.0f;
      const float h = std::max(blend - std::abs(slab - hole), 0.0f) / blend;

      return std::max(slab, hole) + h * h * blend * 0.25f;
    };
  }

  // Uniform in [-1, 1] and never zero, fixed by the seed. The outermost layer is positive, so the surface is closed.
  Field randomField(const vke::VolumeRegion& bounds)
  {
    auto values = std::make_shared<std::map<std::tuple<int, int, int>, float>>();
    std::mt19937 generator(12345);
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

    for (int z = bounds.min.z; z < bounds.max.z; ++z)
    {
      for (int y = bounds.min.y; y < bounds.max.y; ++y)
      {
        for (int x = bounds.min.x; x < bounds.max.x; ++x)
        {
          float value = distribution(generator);
          value = value == 0.0f ? 0.5f : value;

          const bool outer = x == bounds.min.x || y == bounds.min.y || z == bounds.min.z || x == bounds.max.x - 1 ||
                             y == bounds.max.y - 1 || z == bounds.max.z - 1;

          (*values)[{ x, y, z }] = outer ? std::abs(value) : value;
        }
      }
    }

    return [values](const glm::vec3 p) {
      return values->at({ static_cast<int>(p.x), static_cast<int>(p.y), static_cast<int>(p.z) });
    };
  }

  constexpr vke::VolumeRegion s_bounds { { 0, 0, 0 }, { 33, 33, 33 } };

  // Mesh queries

  using EdgeCounts = std::map<std::pair<uint32_t, uint32_t>, int>;

  EdgeCounts directedEdges(const std::vector<uint32_t>& indices)
  {
    EdgeCounts counts;

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
      for (size_t k = 0; k < 3; ++k)
      {
        ++counts[{ indices[i + k], indices[i + (k + 1) % 3] }];
      }
    }

    return counts;
  }

  // Every edge in exactly one triangle per direction: closed, consistently oriented, manifold along edges
  bool isClosedAndOriented(const EdgeCounts& edges)
  {
    return !edges.empty() && std::ranges::all_of(edges, [&edges](const auto& entry) {
      const auto reverse = edges.find({ entry.first.second, entry.first.first });

      return entry.second == 1 && reverse != edges.end() && reverse->second == 1;
    });
  }

  float signedVolume(const vke::SurfaceMesh& mesh)
  {
    float volume = 0.0f;

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
      const glm::vec3& a = mesh.vertices[mesh.indices[i]].position;
      const glm::vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
      const glm::vec3& c = mesh.vertices[mesh.indices[i + 2]].position;

      volume += glm::dot(a, glm::cross(b, c)) / 6.0f;
    }

    return volume;
  }

  bool windsOutward(const vke::SurfaceMesh& mesh)
  {
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
      const vke::SurfaceVertex& a = mesh.vertices[mesh.indices[i]];
      const vke::SurfaceVertex& b = mesh.vertices[mesh.indices[i + 1]];
      const vke::SurfaceVertex& c = mesh.vertices[mesh.indices[i + 2]];

      const glm::vec3 normal = glm::cross(b.position - a.position, c.position - a.position);

      if (glm::length(normal) < 1e-6f)
      {
        continue;
      }

      if (glm::dot(normal, a.normal + b.normal + c.normal) <= 0.0f)
      {
        return false;
      }
    }

    return true;
  }

  float trilinear(const GridSource& source,
                  const glm::vec3 p)
  {
    const glm::ivec3 base = glm::ivec3(glm::floor(p));
    const glm::vec3 f = p - glm::vec3(base);
    float result = 0.0f;

    for (int corner = 0; corner < 8; ++corner)
    {
      const glm::ivec3 offset(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
      const glm::ivec3 position = glm::min(base + offset, s_bounds.max - 1);
      const float weight = (offset.x ? f.x : 1.0f - f.x) * (offset.y ? f.y : 1.0f - f.y) * (offset.z ? f.z : 1.0f - f.z);

      result += weight * source.sampleDistance(position);
    }

    return result;
  }

  // The field interpolated at each vertex vanishes (the vertices sit on grid edges, where it is linear), and so does
  // the analytic field where one is given and the surface is smooth.
  bool vertexOnZeroSet(const GridSource& source,
                       const vke::SurfaceMesh& mesh,
                       const Field* analytic,
                       const float analyticTolerance)
  {
    return std::ranges::all_of(mesh.vertices, [&](const vke::SurfaceVertex& vertex) {
      const glm::vec3 cells = vertex.position / s_cellSize;

      return std::abs(trilinear(source, cells)) < 1e-3f && (analytic == nullptr || std::abs((*analytic)(cells)) < analyticTolerance);
    });
  }

  using VertexKey = std::array<uint32_t, 8>;

  VertexKey keyOf(const vke::SurfaceVertex& vertex)
  {
    static_assert(sizeof(VertexKey) == sizeof(vke::SurfaceVertex));

    VertexKey key {};
    std::memcpy(key.data(), &vertex, sizeof(key));

    return key;
  }

  // Position and normal bits only: the copies of an edge's vertex made for different materials share these
  using GeometryKey = std::array<uint32_t, 6>;

  GeometryKey geometryKeyOf(const vke::SurfaceVertex& vertex)
  {
    static_assert(offsetof(vke::SurfaceVertex, normal) == sizeof(glm::vec3));

    GeometryKey key {};
    std::memcpy(key.data(), &vertex, sizeof(key));

    return key;
  }

  // The meshes' indices with vertices merged by geometry, so a material boundary, which is not welded, does not open
  // the surface
  std::vector<uint32_t> weldedIndices(const std::vector<vke::SurfaceMesh>& meshes)
  {
    std::map<GeometryKey, uint32_t> ids;
    std::vector<uint32_t> indices;

    for (const vke::SurfaceMesh& mesh : meshes)
    {
      for (const uint32_t index : mesh.indices)
      {
        const auto [it, inserted] = ids.emplace(geometryKeyOf(mesh.vertices[index]), static_cast<uint32_t>(ids.size()));
        indices.push_back(it->second);
      }
    }

    return indices;
  }

  std::set<GeometryKey> geometryKeys(const std::vector<vke::SurfaceVertex>& vertices)
  {
    std::set<GeometryKey> keys;

    for (const vke::SurfaceVertex& vertex : vertices)
    {
      keys.insert(geometryKeyOf(vertex));
    }

    return keys;
  }

  std::vector<VertexKey> sortedKeys(const std::vector<vke::SurfaceVertex>& vertices)
  {
    std::vector<VertexKey> keys;

    for (const vke::SurfaceVertex& vertex : vertices)
    {
      keys.push_back(keyOf(vertex));
    }

    std::ranges::sort(keys);

    return keys;
  }

  // Chunk meshes of the whole volume
  std::vector<vke::SurfaceMesh> extractAll(const GridSource& source,
                                           const vke::VolumeChunkGrid& grid)
  {
    std::vector<vke::SurfaceMesh> meshes;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      meshes.push_back(extractChunk(source, grid, chunk));
    }

    return meshes;
  }

  // The vertices of a mesh inside a closed box, in local units
  std::vector<vke::SurfaceVertex> verticesInside(const vke::SurfaceMesh& mesh,
                                                 const glm::vec3 low,
                                                 const glm::vec3 high)
  {
    std::vector<vke::SurfaceVertex> result;

    for (const vke::SurfaceVertex& vertex : mesh.vertices)
    {
      if (glm::all(glm::greaterThanEqual(vertex.position, low)) && glm::all(glm::lessThanEqual(vertex.position, high)))
      {
        result.push_back(vertex);
      }
    }

    return result;
  }

  // Every pair of chunks that touch: the vertices in their common boundary are at bit-identical positions with
  // bit-identical normals. Their materials belong to the triangles on either side, so they are not compared. Returns how
  // many vertices were compared, so a test can tell it did not pass vacuously.
  size_t compareSharedBoundaries(const std::vector<vke::SurfaceMesh>& meshes,
                                 const vke::VolumeChunkGrid& grid,
                                 const std::string& name)
  {
    size_t compared = 0;

    for (uint32_t a = 0; a < meshes.size(); ++a)
    {
      for (uint32_t b = a + 1; b < meshes.size(); ++b)
      {
        const vke::CellRange first = grid.cellRange(a);
        const vke::CellRange second = grid.cellRange(b);
        const glm::ivec3 low = glm::max(first.min, second.min);
        const glm::ivec3 high = glm::min(first.max, second.max);

        if (glm::any(glm::greaterThan(low, high)))
        {
          continue;
        }

        const auto inA = geometryKeys(verticesInside(meshes[a], glm::vec3(low) * s_cellSize, glm::vec3(high) * s_cellSize));
        const auto inB = geometryKeys(verticesInside(meshes[b], glm::vec3(low) * s_cellSize, glm::vec3(high) * s_cellSize));

        check(inA == inB, name + ": chunks " + std::to_string(a) + " and " + std::to_string(b) + " agree on their shared vertices");
        compared += inA.size();
      }
    }

    return compared;
  }

  size_t triangleCount(const std::vector<vke::SurfaceMesh>& meshes)
  {
    size_t count = 0;

    for (const vke::SurfaceMesh& mesh : meshes)
    {
      count += mesh.indices.size() / 3;
    }

    return count;
  }

  // The chunks' vertices together are exactly the vertices of one chunk covering the volume, with the same triangles
  void compareWithSingleChunk(const GridSource& source,
                              const std::vector<vke::SurfaceMesh>& meshes,
                              const std::string& name)
  {
    const vke::VolumeChunkGrid single(source.bounds(), 64);
    const vke::SurfaceMesh whole = extractChunk(source, single, 0);

    std::set<VertexKey> chunked;

    for (const vke::SurfaceMesh& mesh : meshes)
    {
      for (const vke::SurfaceVertex& vertex : mesh.vertices)
      {
        chunked.insert(keyOf(vertex));
      }
    }

    const auto wholeKeys = sortedKeys(whole.vertices);

    check(single.chunkCount() == 1, name + ": one chunk covers the volume");
    check(std::set<VertexKey>(wholeKeys.begin(), wholeKeys.end()) == chunked, name + ": chunked vertices equal the single-chunk vertices");
    check(triangleCount(meshes) == whole.indices.size() / 3, name + ": chunked triangle count equals the single-chunk count");
  }

  // The whole volume's meshes merged by vertex geometry: closed and oriented across chunk borders
  void checkMergedClosed(const std::vector<vke::SurfaceMesh>& meshes,
                         const std::string& name)
  {
    check(isClosedAndOriented(directedEdges(weldedIndices(meshes))), name + ": merged chunks form a closed, oriented surface");
  }

  struct Case {
    std::string name;
    Field field;
    bool closed;
    bool analytic;
  };

  std::vector<Case> makeCases()
  {
    return {
      { "sphere", sphereField(), true, true },
      { "plane", planeField(), false, true },
      { "torus", torusField(), true, true },
      { "slab with a hole", slabField(), false, false },
      { "random", randomField(s_bounds), true, false }
    };
  }

  void checkSingleChunk(const Case& testCase)
  {
    const GridSource source(s_bounds, testCase.field);
    const vke::VolumeChunkGrid grid(s_bounds, 64);
    const vke::SurfaceMesh mesh = extractChunk(source, grid, 0);

    check(!mesh.indices.empty() && mesh.indices.size() % 3 == 0, testCase.name + ": has triangles");

    check(std::ranges::all_of(mesh.indices, [&mesh](const uint32_t index) { return index < mesh.vertices.size(); }),
          testCase.name + ": indices are in range");

    if (testCase.closed)
    {
      check(isClosedAndOriented(directedEdges(weldedIndices({ mesh }))), testCase.name + ": closed and consistently oriented");
      check(signedVolume(mesh) > 0.0f, testCase.name + ": winds outward (positive enclosed volume)");
    }

    if (testCase.name != "random")
    {
      check(windsOutward(mesh), testCase.name + ": triangle normals agree with vertex normals");
    }

    check(vertexOnZeroSet(source, mesh, testCase.analytic ? &testCase.field : nullptr, 0.05f), testCase.name + ": vertices are on the zero set");

    check(std::ranges::all_of(mesh.vertices, [](const vke::SurfaceVertex& vertex) {
      return std::abs(glm::length(vertex.normal) - 1.0f) < 1e-4f;
    }), testCase.name + ": vertex normals are unit length");
  }

  void checkChunked(const Case& testCase)
  {
    const GridSource source(s_bounds, testCase.field);
    const vke::VolumeChunkGrid grid(s_bounds, 16);
    const auto meshes = extractAll(source, grid);

    check(grid.chunkCount() == 8, testCase.name + ": 2 x 2 x 2 chunks");
    check(compareSharedBoundaries(meshes, grid, testCase.name) > 0, testCase.name + ": chunks share vertices");

    compareWithSingleChunk(source, meshes, testCase.name);

    if (testCase.closed)
    {
      checkMergedClosed(meshes, testCase.name);
    }
  }

  // Two chunks side by side on a volume whose edge clips the second one's fetch region
  void checkEdgeClamping(const Case& testCase)
  {
    const vke::VolumeRegion bounds { { 0, 0, 0 }, { 30, 33, 33 } };
    const GridSource source(bounds, testCase.field);
    const vke::VolumeChunkGrid grid(bounds, 16);

    check(grid.chunkCounts().x == 2, testCase.name + ": uneven volume has two chunks along x");

    const vke::VolumeRegion fetch = grid.fetchRegion(1);

    check(fetch.max.x == bounds.max.x, testCase.name + ": the last chunk's fetch region is clipped to the bounds");

    const auto meshes = extractAll(source, grid);

    check(compareSharedBoundaries(meshes, grid, testCase.name + " (edge)") > 0, testCase.name + ": edge chunks share vertices");

    compareWithSingleChunk(source, meshes, testCase.name + " (edge)");
  }

  // Whether the vertex lies strictly inside an edge along y or z (a vertex on an x edge, or on a sample, has both
  // other coordinates on the lattice). Cell coordinates are exact, as the cell size is a power of two.
  bool onYOrZEdge(const vke::SurfaceVertex& vertex)
  {
    const glm::vec3 cells = vertex.position / s_cellSize;

    return cells.y != std::floor(cells.y) || cells.z != std::floor(cells.z);
  }

  // Every distinct vertex of the meshes on the plane x = 32 (local x = 16), by position, with its normal's bits. Only
  // vertices on y and z edges: an x edge ending at the plane (crossing at t == 1) is meshed by one side only.
  std::map<std::array<float, 3>, std::array<uint32_t, 3>> seamVertices(const std::vector<vke::SurfaceMesh>& meshes)
  {
    std::map<std::array<float, 3>, std::array<uint32_t, 3>> seam;

    for (const vke::SurfaceMesh& mesh : meshes)
    {
      for (const vke::SurfaceVertex& vertex : mesh.vertices)
      {
        if (vertex.position.x == 32.0f * s_cellSize && onYOrZEdge(vertex))
        {
          std::array<uint32_t, 3> normal {};
          std::memcpy(normal.data(), &vertex.normal, sizeof(normal));

          seam.emplace(std::array<float, 3> { vertex.position.x, vertex.position.y, vertex.position.z }, normal);
        }
      }
    }

    return seam;
  }

  vke::VolumeChunkGrid gridOf(const vke::VolumeSource& source,
                              const uint32_t chunkCells)
  {
    return { source.bounds(), source.sampleBounds(), chunkCells };
  }

  [[nodiscard]] std::set<std::array<float, 3>> seamPositions(const std::map<std::array<float, 3>, std::array<uint32_t, 3>>& seam)
  {
    std::set<std::array<float, 3>> result;

    for (const auto& entry : seam)
    {
      result.insert(entry.first);
    }

    return result;
  }

  [[nodiscard]] float seamField(const glm::vec3 p)
  {
    return glm::length(p - glm::vec3(32.4f, 12.3f, 11.7f)) - 9.3f;
  }

  // Two surfaces placed edge to edge over one curved field, sharing the sample plane x = 32. Both compute a seam vertex
  // from identical sample values with identical operations, so exact comparison is valid.
  void checkSeamNormals()
  {
    const vke::VolumeRegion left { { 0, 0, 0 }, { 33, 25, 25 } };
    const vke::VolumeRegion right { { 32, 0, 0 }, { 65, 25, 25 } };
    const vke::VolumeRegion leftSamples { { 0, 0, 0 }, { 34, 25, 25 } };
    const vke::VolumeRegion rightSamples { { 31, 0, 0 }, { 65, 25, 25 } };
    const vke::VolumeRegion whole { { 0, 0, 0 }, { 65, 25, 25 } };

    const Field field = seamField;

    const GridSource wideLeft(left, leftSamples, field);
    const GridSource wideRight(right, rightSamples, field);
    const GridSource single(whole, field);

    const auto leftSeam = seamVertices(extractAll(wideLeft, gridOf(wideLeft, 16)));
    const auto rightSeam = seamVertices(extractAll(wideRight, gridOf(wideRight, 16)));
    const auto wholeSeam = seamVertices(extractAll(single, gridOf(single, 24)));

    check(leftSeam.size() > 8, "seam: the sphere crosses the shared plane");
    check(leftSeam == rightSeam, "seam: both sides produce identical vertices on the shared plane with wider sample bounds");
    check(leftSeam == wholeSeam, "seam: the shared plane's vertices equal those of a single surface");

    // Without the wider sample bounds the normals at the shared faces are one-sided differences, which disagree
    const GridSource narrowLeft(left, field);
    const GridSource narrowRight(right, field);

    const auto narrowLeftSeam = seamVertices(extractAll(narrowLeft, gridOf(narrowLeft, 16)));
    const auto narrowRightSeam = seamVertices(extractAll(narrowRight, gridOf(narrowRight, 16)));

    check(seamPositions(narrowLeftSeam) == seamPositions(leftSeam) && seamPositions(narrowRightSeam) == seamPositions(leftSeam),
          "seam canary: positions on the shared plane do not depend on the sample bounds");
    check(narrowLeftSeam != narrowRightSeam, "seam canary: without wider sample bounds the shared normals disagree");
  }

  // The extra samples feed normals only: nothing is meshed past the bounds' cells
  void checkApronMeshesNothingExtra()
  {
    const vke::VolumeRegion right { { 32, 0, 0 }, { 65, 25, 25 } };
    const vke::VolumeRegion rightSamples { { 31, 0, 0 }, { 65, 25, 25 } };
    const Field field = seamField;
    const GridSource wideRight(right, rightSamples, field);
    const GridSource narrowRight(right, field);

    const auto wideMeshes = extractAll(wideRight, gridOf(wideRight, 16));
    const auto narrowMeshes = extractAll(narrowRight, gridOf(narrowRight, 16));
    const glm::vec3 low = glm::vec3(right.min) * s_cellSize;
    const glm::vec3 high = glm::vec3(right.max - 1) * s_cellSize;

    size_t wideVertices = 0;
    size_t narrowVertices = 0;
    size_t wideIndices = 0;
    size_t narrowIndices = 0;
    bool inside = true;

    for (const vke::SurfaceMesh& mesh : wideMeshes)
    {
      wideVertices += mesh.vertices.size();
      wideIndices += mesh.indices.size();
      inside = inside && verticesInside(mesh, low, high).size() == mesh.vertices.size();
    }

    for (const vke::SurfaceMesh& mesh : narrowMeshes)
    {
      narrowVertices += mesh.vertices.size();
      narrowIndices += mesh.indices.size();
    }

    check(wideVertices > 0, "wide bounds: the surface has vertices");
    check(inside, "wide bounds: every vertex lies inside the bounds' cells");
    check(wideVertices == narrowVertices && wideIndices == narrowIndices,
          "wide bounds: the vertex and index counts equal those of the default sample bounds");
  }

  void checkDeterministic()
  {
    const GridSource source(s_bounds, randomField(s_bounds));
    const vke::VolumeChunkGrid grid(s_bounds, 16);

    const vke::SurfaceMesh first = extractChunk(source, grid, 3);

    // Another chunk in between, so the worker's reused scratch is dirty when the first is extracted again
    const vke::SurfaceMesh other = extractChunk(source, grid, 5);
    const vke::SurfaceMesh second = extractChunk(source, grid, 3);

    check(!other.indices.empty(), "determinism: the other chunk has triangles");
    check(first.indices == second.indices, "determinism: indices repeat");
    check(first.vertices.size() == second.vertices.size() &&
            std::memcmp(first.vertices.data(), second.vertices.data(), first.vertices.size() * sizeof(vke::SurfaceVertex)) == 0,
          "determinism: vertices repeat bit for bit");
  }

  void checkEmptyAndFull()
  {
    const vke::VolumeChunkGrid grid(s_bounds, 16);

    const GridSource empty(s_bounds, [](const glm::vec3&) { return 1.0f; });
    const GridSource full(s_bounds, [](const glm::vec3&) { return -1.0f; });
    const GridSource zero(s_bounds, [](const glm::vec3&) { return 0.0f; });

    bool anyGeometry = false;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      for (const GridSource* source : { &empty, &full, &zero })
      {
        const vke::SurfaceMesh mesh = extractChunk(*source, grid, chunk);

        anyGeometry = anyGeometry || !mesh.indices.empty() || !mesh.vertices.empty();
      }
    }

    check(!anyGeometry, "all-positive, all-negative and all-zero chunks produce no geometry");
  }

  void checkPlaneSplitsChunks()
  {
    // A plane at x = 8.5 crosses only the chunks of the first column, the rest hold nothing
    const GridSource source(s_bounds, [](const glm::vec3& p) { return p.x - 8.5f; });
    const vke::VolumeChunkGrid grid(s_bounds, 16);

    size_t nonEmpty = 0;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      const bool hasGeometry = !extractChunk(source, grid, chunk).indices.empty();
      const bool firstColumn = grid.cellRange(chunk).min.x == 0;

      check(hasGeometry == firstColumn, "plane: geometry only in the chunks it crosses");
      nonEmpty += hasGeometry ? 1 : 0;
    }

    check(nonEmpty == 4, "plane: the four chunks of the first column");
  }

  // One weight is 255 and the rest are 0
  bool weightsOneHot(const vke::SurfaceVertex& vertex)
  {
    return std::ranges::count(vertex.weights, uint8_t { 255 }) == 1 && std::ranges::count(vertex.weights, uint8_t { 0 }) == 3;
  }

  // The material in the slot with the full weight, or -1 when there is none
  int ownMaterial(const vke::SurfaceVertex& vertex)
  {
    const auto full = std::ranges::find(vertex.weights, uint8_t { 255 });

    return full == vertex.weights.end() ? -1 : vertex.materials[static_cast<size_t>(full - vertex.weights.begin())];
  }

  // The corners list the same materials in the same slots: exactly their own materials, ascending, then empty slots.
  // Sets count to how many materials the triangle has.
  bool triangleSlotsValid(const std::array<const vke::SurfaceVertex*, 3>& corners,
                          size_t& count)
  {
    std::set<int> own;

    for (const vke::SurfaceVertex* corner : corners)
    {
      own.insert(ownMaterial(*corner));
    }

    count = own.size();

    std::array<uint8_t, 4> expected {};
    std::ranges::copy(own, expected.begin());

    const auto& materials = corners[0]->materials;

    return !own.contains(-1) && materials == corners[1]->materials && materials == corners[2]->materials && materials == expected;
  }

  // Every vertex's weights are one-hot, so they sum to 255, and the corners of each triangle list its materials in the
  // same slots (triangleSlotsValid). Returns how many triangles have two and three materials.
  std::array<size_t, 2> checkMaterialSlots(const vke::SurfaceMesh& mesh,
                                           const std::string& name)
  {
    std::array<size_t, 2> blended {};
    bool triangleValid = true;

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
      size_t count = 0;

      triangleValid = triangleSlotsValid({ &mesh.vertices[mesh.indices[i]], &mesh.vertices[mesh.indices[i + 1]], &mesh.vertices[mesh.indices[i + 2]] }, count) &&
                      triangleValid;

      if (count > 1 && count < 4)
      {
        ++blended[count - 2];
      }
    }

    check(std::ranges::all_of(mesh.vertices, weightsOneHot), name + ": every vertex has one full weight, summing to one");
    check(triangleValid, name + ": a triangle's corners list its materials in the same slots");

    return blended;
  }

  // Each vertex's own material is its edge's inside sample's, read back from the source: the edge is the axis along which
  // the vertex's cell coordinate is not whole. Vertices exactly on a sample (an ambiguous edge) are skipped. Returns how
  // many vertices were checked.
  size_t checkOwnMaterials(const GridSource& source,
                           const vke::SurfaceMesh& mesh,
                           const std::string& name)
  {
    size_t checked = 0;
    bool valid = true;

    for (const vke::SurfaceVertex& vertex : mesh.vertices)
    {
      const glm::vec3 cells = vertex.position / s_cellSize;
      const glm::vec3 lowerCells = glm::floor(cells);
      int axis = -1;

      for (int i = 0; i < 3; ++i)
      {
        axis = cells[i] != lowerCells[i] ? i : axis;
      }

      if (axis < 0)
      {
        continue;
      }

      const glm::ivec3 lower(lowerCells);
      glm::ivec3 upper = lower;
      upper[axis] += 1;

      const uint8_t expected = source.sampleMaterial(source.sampleDistance(lower) < 0.0f ? lower : upper);

      valid = valid && ownMaterial(vertex) == expected;

      ++checked;
    }

    check(valid, name + ": every vertex's own material is its edge's inside sample's");

    return checked;
  }

  // No two vertices are equal in every byte: triangles that share an edge and a material set share its vertex
  void checkWelded(const vke::SurfaceMesh& mesh,
                   const std::string& name)
  {
    const auto keys = sortedKeys(mesh.vertices);

    check(std::ranges::adjacent_find(keys) == keys.end(), name + ": vertices with the same edge and materials are welded");
  }

  // A sphere across three materials: x below 16 is material 1, above it y below 16 is 2, the rest 3. The sphere is
  // centered near (16, 16, 16), so the line where all three meet crosses it, which gives three-material triangles.
  void checkMaterialBoundaries()
  {
    const GridSource source(s_bounds, sphereField(), [](const glm::ivec3 p) {
      return static_cast<uint8_t>(p.x < 16 ? 1 : p.y < 16 ? 2 : 3);
    });

    const vke::VolumeChunkGrid single(s_bounds, 64);
    const vke::SurfaceMesh mesh = extractChunk(source, single, 0);
    const auto blended = checkMaterialSlots(mesh, "boundaries");

    check(blended[0] > 0, "boundaries: some triangles blend two materials");
    check(blended[1] > 0, "boundaries: some triangles blend three materials");
    check(checkOwnMaterials(source, mesh, "boundaries") > 0, "boundaries: own materials were checked");
    checkWelded(mesh, "boundaries");
    check(isClosedAndOriented(directedEdges(weldedIndices({ mesh }))), "boundaries: the surface stays closed across material boundaries");

    // Each copy of an edge's vertex has the same own material, its edge's inside sample's
    std::map<GeometryKey, std::set<int>> ownByVertex;

    for (const vke::SurfaceVertex& vertex : mesh.vertices)
    {
      ownByVertex[geometryKeyOf(vertex)].insert(ownMaterial(vertex));
    }

    check(geometryKeys(mesh.vertices).size() < mesh.vertices.size(), "boundaries: material boundaries duplicate vertices");
    check(std::ranges::all_of(ownByVertex, [](const auto& entry) { return entry.second.size() == 1; }),
          "boundaries: every copy of a vertex has the same own material");

    // Chunked: each chunk's triangles carry the same materials as the single chunk's
    const vke::VolumeChunkGrid grid(s_bounds, 16);
    const auto meshes = extractAll(source, grid);

    for (uint32_t chunk = 0; chunk < meshes.size(); ++chunk)
    {
      checkMaterialSlots(meshes[chunk], "boundaries chunk " + std::to_string(chunk));
      checkOwnMaterials(source, meshes[chunk], "boundaries chunk " + std::to_string(chunk));
      checkWelded(meshes[chunk], "boundaries chunk " + std::to_string(chunk));
    }

    compareWithSingleChunk(source, meshes, "boundaries");
    checkMergedClosed(meshes, "boundaries");
  }

  // One material everywhere: every vertex has its full weight on the first slot and nothing else, and the edges'
  // vertices are welded as before
  void checkSingleMaterial()
  {
    const GridSource source(s_bounds, torusField(), [](const glm::ivec3) { return static_cast<uint8_t>(3); });
    const vke::VolumeChunkGrid grid(s_bounds, 64);
    const vke::SurfaceMesh mesh = extractChunk(source, grid, 0);

    check(!mesh.vertices.empty(), "single material: has vertices");
    check(std::ranges::all_of(mesh.vertices, [](const vke::SurfaceVertex& vertex) {
      return vertex.materials == std::array<uint8_t, 4> { 3, 0, 0, 0 } && vertex.weights == std::array<uint8_t, 4> { 255, 0, 0, 0 };
    }), "single material: weight one on the first slot only");
    check(geometryKeys(mesh.vertices).size() == mesh.vertices.size(), "single material: one vertex per crossing edge");
    check(isClosedAndOriented(directedEdges(mesh.indices)), "single material: closed and oriented by index");
  }

  // Bourke's corner order: bits 0 to 3 go around z = 0 from the origin, bits 4 to 7 sit above them
  constexpr std::array<std::array<int, 3>, 8> s_corners {{
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 }
  }};

  // One cell whose corners are inside (-1) or outside (+1) as the bits of the pattern say
  Field patternField(const unsigned pattern)
  {
    return [pattern](const glm::vec3 p) {
      for (unsigned corner = 0; corner < 8; ++corner)
      {
        if (static_cast<int>(p.x) == s_corners[corner][0] && static_cast<int>(p.y) == s_corners[corner][1] &&
            static_cast<int>(p.z) == s_corners[corner][2])
        {
          return (pattern >> corner & 1u) != 0 ? -1.0f : 1.0f;
        }
      }

      return 1.0f;
    };
  }

  // The midpoint of every edge of the 2 x 2 x 2 samples whose ends differ in sign, found from the signs alone
  std::set<std::array<float, 3>> crossingMidpoints(const GridSource& source)
  {
    std::set<std::array<float, 3>> midpoints;

    for (int axis = 0; axis < 3; ++axis)
    {
      for (int base = 0; base < 4; ++base)
      {
        glm::ivec3 lower;
        lower[axis] = 0;
        lower[(axis + 1) % 3] = base & 1;
        lower[(axis + 2) % 3] = base >> 1;

        glm::ivec3 upper = lower;
        upper[axis] = 1;

        if ((source.sampleDistance(lower) < 0.0f) != (source.sampleDistance(upper) < 0.0f))
        {
          const glm::vec3 position = (glm::vec3(lower) + 0.5f * glm::vec3(upper - lower)) * s_cellSize;
          midpoints.insert({ position.x, position.y, position.z });
        }
      }
    }

    return midpoints;
  }

  // Each of the 256 corner sign patterns on a single cell gives a vertex on the midpoint of every crossing edge, and
  // triangles only where the cell is neither fully inside nor outside.
  void checkAllCubeCases()
  {
    const vke::VolumeRegion cell { { 0, 0, 0 }, { 2, 2, 2 } };
    const vke::VolumeChunkGrid grid(cell, 1);

    for (unsigned pattern = 0; pattern < 256; ++pattern)
    {
      const GridSource source(cell, patternField(pattern));
      const vke::SurfaceMesh mesh = extractChunk(source, grid, 0);

      std::set<std::array<float, 3>> actual;

      for (const vke::SurfaceVertex& vertex : mesh.vertices)
      {
        actual.insert({ vertex.position.x, vertex.position.y, vertex.position.z });
      }

      const std::string name = "cube case " + std::to_string(pattern);

      check(actual == crossingMidpoints(source) && actual.size() == mesh.vertices.size(), name + ": a vertex per crossing edge");
      check(mesh.indices.empty() == (pattern == 0 || pattern == 255), name + ": triangles unless uniform");

      // Every corner its own material, so every triangle of the table blends: the same surface, with valid slots, each
      // vertex owning its inside sample's material, and copies only per material set
      const GridSource mixed(cell, patternField(pattern), [](const glm::ivec3 p) {
        return static_cast<uint8_t>(p.x + 2 * p.y + 4 * p.z);
      });
      const vke::SurfaceMesh mixedMesh = extractChunk(mixed, grid, 0);

      check(geometryKeys(mixedMesh.vertices) == geometryKeys(mesh.vertices) && mixedMesh.indices.size() == mesh.indices.size(),
            name + " (mixed): the same surface");
      checkMaterialSlots(mixedMesh, name + " (mixed)");
      checkOwnMaterials(mixed, mixedMesh, name + " (mixed)");
      checkWelded(mixedMesh, name + " (mixed)");
    }
  }

  // The random field has every cube case, so the closed-surface checks above cover the whole table
  void checkRandomFieldCoversAllCases()
  {
    const GridSource source(s_bounds, randomField(s_bounds));
    std::set<unsigned> seen;

    for (int z = 0; z < 32; ++z)
    {
      for (int y = 0; y < 32; ++y)
      {
        for (int x = 0; x < 32; ++x)
        {
          unsigned index = 0;

          for (unsigned corner = 0; corner < 8; ++corner)
          {
            const glm::ivec3 position(x + s_corners[corner][0], y + s_corners[corner][1], z + s_corners[corner][2]);
            index |= source.sampleDistance(position) < 0.0f ? 1u << corner : 0u;
          }

          seen.insert(index);
        }
      }
    }

    check(seen.size() == 256, "the random field contains all 256 corner patterns");
  }

} // namespace

int main()
{
  checkRandomFieldCoversAllCases();
  checkAllCubeCases();

  for (const Case& testCase : makeCases())
  {
    checkSingleChunk(testCase);
    checkChunked(testCase);
    checkEdgeClamping(testCase);
  }

  checkSeamNormals();
  checkApronMeshesNothingExtra();
  checkDeterministic();
  checkEmptyAndFull();
  checkPlaneSplitsChunks();
  checkMaterialBoundaries();
  checkSingleMaterial();

  std::cout << g_checks << " checks, " << g_failures << " failures" << std::endl;

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
