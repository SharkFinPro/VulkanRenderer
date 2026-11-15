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

  constexpr float s_cellSize = 0.5f;

  // A field sampled into memory. Requests outside the bounds are failures, as the engine never makes them.
  class GridSource final : public vke::VolumeSource {
  public:
    GridSource(const vke::VolumeRegion& bounds,
               const Field& field)
      : m_bounds(bounds),
        m_size(vke::extent(bounds)),
        m_distance(vke::sampleCount(bounds)),
        m_material(vke::sampleCount(bounds))
    {
      for (int z = 0; z < m_size.z; ++z)
      {
        for (int y = 0; y < m_size.y; ++y)
        {
          for (int x = 0; x < m_size.x; ++x)
          {
            const glm::ivec3 position = bounds.min + glm::ivec3(x, y, z);
            const size_t index = indexOf(position);

            m_distance[index] = field(glm::vec3(position));
            m_material[index] = static_cast<uint8_t>((position.x / 7 + position.y / 5 + position.z / 3) % 5);
          }
        }
      }
    }

    void fill(const vke::VolumeRegion& region,
              uint32_t,
              std::span<float> distance,
              std::span<uint8_t> material) const override
    {
      const vke::VolumeRegion inside = vke::intersect(region, m_bounds);
      check(inside.min == region.min && inside.max == region.max, "fill stays inside the bounds");
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

    [[nodiscard]] float sampleDistance(const glm::ivec3 position) const
    {
      return m_distance[indexOf(position)];
    }

  private:
    vke::VolumeRegion m_bounds;
    glm::ivec3 m_size;
    std::vector<float> m_distance;
    std::vector<uint8_t> m_material;

    [[nodiscard]] size_t indexOf(const glm::ivec3 position) const
    {
      const glm::ivec3 local = position - m_bounds.min;

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

  using VertexKey = std::array<uint32_t, 7>;

  VertexKey keyOf(const vke::SurfaceVertex& vertex)
  {
    static_assert(sizeof(VertexKey) == sizeof(vke::SurfaceVertex));

    VertexKey key {};
    std::memcpy(key.data(), &vertex, sizeof(key));

    return key;
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

  // Every pair of chunks that touch: the vertices in their common boundary are bit-identical, in equal number. Returns
  // how many vertices were compared, so a test can tell it did not pass vacuously.
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

        const auto inA = sortedKeys(verticesInside(meshes[a], glm::vec3(low) * s_cellSize, glm::vec3(high) * s_cellSize));
        const auto inB = sortedKeys(verticesInside(meshes[b], glm::vec3(low) * s_cellSize, glm::vec3(high) * s_cellSize));

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

  // The whole volume's meshes merged by vertex bits: closed and oriented across chunk borders
  void checkMergedClosed(const std::vector<vke::SurfaceMesh>& meshes,
                         const std::string& name)
  {
    std::map<VertexKey, uint32_t> ids;
    std::vector<uint32_t> indices;

    for (const vke::SurfaceMesh& mesh : meshes)
    {
      for (const uint32_t index : mesh.indices)
      {
        const auto [it, inserted] = ids.emplace(keyOf(mesh.vertices[index]), static_cast<uint32_t>(ids.size()));
        indices.push_back(it->second);
      }
    }

    check(isClosedAndOriented(directedEdges(indices)), name + ": merged chunks form a closed, oriented surface");
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
      check(isClosedAndOriented(directedEdges(mesh.indices)), testCase.name + ": closed and consistently oriented");
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

  checkDeterministic();
  checkEmptyAndFull();
  checkPlaneSplitsChunks();

  std::cout << g_checks << " checks, " << g_failures << " failures" << std::endl;

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
