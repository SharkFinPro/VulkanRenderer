// Headless checks of VolumeChunkGrid (the chunk grid and its 1-before, 2-after sample border) and VolumeScheduler (the
// batch bookkeeping behind a volume surface). No engine, no GPU, no threads: ctest runs this. The tests set job phases
// by hand where a worker would.
#include "source/components/assets/volume/VolumeChunking.h"
#include "source/components/assets/volume/VolumeScheduler.h"
#include <glm/vec3.hpp>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
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

  using Chunks = std::vector<uint32_t>;

  Chunks touched(const vke::VolumeChunkGrid& grid,
                 const vke::VolumeRegion& region)
  {
    Chunks out { 999, 998 };
    grid.touchedChunks(region, out);

    return out;
  }

  vke::VolumeRegion sampleRegion(const glm::ivec3 position)
  {
    return { position, position + 1 };
  }

  template <typename F>
  bool throwsInvalidArgument(F&& construct)
  {
    try
    {
      construct();
    }
    catch (const std::invalid_argument&)
    {
      return true;
    }

    return false;
  }

  void checkGridCounts()
  {
    const vke::VolumeChunkGrid exact({ { 0, 0, 0 }, { 65, 65, 65 } }, 32);
    check(exact.chunkCounts() == glm::uvec3(2) && exact.chunkCount() == 8, "64 cells in chunks of 32 make 2 per axis");

    const vke::VolumeChunkGrid uneven({ { -5, 3, 10 }, { 61, 40, 12 } }, 32);
    // 65 x 36 x 1 cells
    check(uneven.chunkCounts() == glm::uvec3(3, 2, 1) && uneven.chunkCount() == 6, "uneven sizes round up");

    const vke::VolumeChunkGrid oneChunk({ { 0, 0, 0 }, { 33, 2, 2 } }, 32);
    check(oneChunk.chunkCount() == 1, "exactly one chunk of cells is one chunk");

    check(vke::VolumeChunkGrid({ { 0, 0, 0 }, { 1, 40, 40 } }, 32).chunkCount() == 0, "one sample along an axis has no cells");
    check(vke::VolumeChunkGrid({ { 0, 0, 0 }, { 40, 0, 40 } }, 32).chunkCount() == 0, "no samples along an axis has no chunks");
    check(vke::VolumeChunkGrid({ { 0, 0, 0 }, { 0, 0, 0 } }, 32).chunkCount() == 0, "an empty volume has no chunks");

    check(throwsInvalidArgument([] { vke::VolumeChunkGrid({ { 0, 0, 0 }, { 8, 8, 8 } }, 0); }), "chunk size 0 throws");
    check(throwsInvalidArgument([] { vke::VolumeChunkGrid({ { 0, 0, 0 }, { 8, 8, 8 } }, 65); }), "chunk size 65 throws");
    check(!throwsInvalidArgument([] { vke::VolumeChunkGrid({ { 0, 0, 0 }, { 8, 8, 8 } }, 64); }), "chunk size 64 is allowed");
    check(!throwsInvalidArgument([] { vke::VolumeChunkGrid({ { 0, 0, 0 }, { 8, 8, 8 } }, 1); }), "chunk size 1 is allowed");
    check(throwsInvalidArgument([] { vke::VolumeChunkGrid({ { 0, 0, 0 }, { 8, -1, 8 } }, 8); }), "max below min throws");
  }

  void checkCellRangesAndFetchRegions()
  {
    const vke::VolumeChunkGrid grid({ { 0, 0, 0 }, { 66, 129, 2 } }, 32);

    // 65 x 128 x 1 cells: 3 x 4 x 1 chunks, indexed x-fastest
    check(grid.chunkCounts() == glm::uvec3(3, 4, 1), "counts for the ranges test");

    const vke::CellRange first = grid.cellRange(0);
    check(first.min == glm::ivec3(0) && first.max == glm::ivec3(32, 32, 1), "first chunk's cells");

    const vke::CellRange last = grid.cellRange(2);
    check(last.min == glm::ivec3(64, 0, 0) && last.max == glm::ivec3(65, 32, 1), "the last chunk along x is clipped to the cells");

    const vke::CellRange row = grid.cellRange(3);
    check(row.min == glm::ivec3(0, 32, 0), "chunks are indexed x-fastest");

    const vke::VolumeRegion needed = grid.neededSamples(0);
    check(needed.min == glm::ivec3(-1, -1, -1) && needed.max == glm::ivec3(34, 34, 3), "needed samples reach one before and two after");

    const vke::VolumeRegion fetchFirst = grid.fetchRegion(0);
    check(fetchFirst.min == glm::ivec3(0) && fetchFirst.max == glm::ivec3(34, 34, 2), "the fetch region is clipped to the bounds");

    const vke::VolumeRegion fetchInterior = grid.fetchRegion(1 + 3);
    check(fetchInterior.min == glm::ivec3(31, 31, 0) && fetchInterior.max == glm::ivec3(66, 66, 2),
          "an interior chunk fetches its full border");

    const vke::VolumeRegion fetchLast = grid.fetchRegion(2 + 3 * 3);
    check(fetchLast.max == glm::ivec3(66, 129, 2), "the last chunk's fetch region ends at the bounds");

    const vke::AABB localBounds = grid.localBounds(1, 0.5f);
    check(localBounds.min == glm::vec3(16.0f, 0.0f, 0.0f) && localBounds.max == glm::vec3(32.0f, 16.0f, 0.5f),
          "local bounds are the cell range times the cell size");
  }

  void checkTouchedChunksSingleSample()
  {
    const vke::VolumeChunkGrid grid({ { 0, 0, 0 }, { 129, 129, 129 } }, 32);

    // Chunk 0 needs samples -1 to 33 along x, chunk 1 needs 31 to 65: 31 is chunk 1's one-before, 33 is chunk 0's two-after
    const auto atX = [&grid](const int x) {
      return touched(grid, sampleRegion({ x, 40, 40 }));
    };

    const uint32_t row = 4 + 4 * 4 + 0;
    const auto chunksOf = [row](const std::vector<uint32_t>& xs) {
      Chunks expected;

      for (const uint32_t x : xs)
      {
        expected.push_back(row + x);
      }

      return expected;
    };

    check(atX(30) == chunksOf({ 0 }), "x = 30 touches only chunk 0");
    check(atX(31) == chunksOf({ 0, 1 }), "x = 31 (one before chunk 1's cells) touches chunks 0 and 1");
    check(atX(32) == chunksOf({ 0, 1 }), "x = 32 touches chunks 0 and 1");
    check(atX(33) == chunksOf({ 0, 1 }), "x = 33 (two after chunk 0's cells) touches chunks 0 and 1");
    check(atX(34) == chunksOf({ 1 }), "x = 34 touches only chunk 1");
    check(atX(40) == chunksOf({ 1 }), "an interior sample touches one chunk");

    check(touched(grid, sampleRegion({ 33, 33, 33 })).size() == 8, "a sample past every chunk's corner touches all 8 around it");
    check(touched(grid, sampleRegion({ 33, 40, 40 })) == Chunks({ 20, 21 }), "linear indices are x-fastest");
    check(touched(grid, sampleRegion({ 40, 33, 40 })) == Chunks({ 17, 21 }), "the y neighbor is one row of chunks away");
    check(touched(grid, sampleRegion({ 40, 40, 33 })) == Chunks({ 5, 21 }), "the z neighbor is one layer of chunks away");
  }

  void checkTouchedChunksRegions()
  {
    const vke::VolumeChunkGrid grid({ { 0, 0, 0 }, { 129, 129, 129 } }, 32);

    check(touched(grid, sampleRegion({ 32, 0, 0 })) == Chunks({ 0, 1 }), "a region on a chunk boundary touches both sides");
    check(touched(grid, { { 32, 0, 0 }, { 64, 1, 1 } }) == Chunks({ 0, 1, 2 }), "a region spanning a chunk touches its neighbors too");

    check(touched(grid, { { -10, -10, -10 }, { 5, 5, 5 } }) == Chunks({ 0 }), "a region partly below the bounds is clipped");
    check(touched(grid, { { 120, 120, 120 }, { 200, 200, 200 } }) == Chunks({ 63 }), "a region partly above the bounds is clipped");
    check(touched(grid, { { 200, 0, 0 }, { 300, 5, 5 } }).empty(), "a region outside the bounds touches nothing");
    check(touched(grid, { { 5, 5, 5 }, { 5, 9, 9 } }).empty(), "an empty region touches nothing");
    check(touched(grid, { { 9, 9, 9 }, { 5, 5, 5 } }).empty(), "an inverted region touches nothing");

    const Chunks all = touched(grid, grid.getBounds());
    check(all.size() == 64 && std::ranges::is_sorted(all), "the whole volume touches every chunk, ascending");

    check(touched(vke::VolumeChunkGrid({ { 0, 0, 0 }, { 1, 1, 1 } }, 32), { { 0, 0, 0 }, { 1, 1, 1 } }).empty(),
          "a grid without chunks touches nothing");
  }

  void checkDefaultSampleBounds()
  {
    const vke::VolumeRegion bounds { { 0, 0, 0 }, { 129, 129, 129 } };
    const vke::VolumeChunkGrid grid(bounds, 32);

    check(grid.getSampleBounds().min == bounds.min && grid.getSampleBounds().max == bounds.max, "the default sample bounds are the bounds");

    const vke::VolumeRegion fetchFirst = grid.fetchRegion(0);
    check(fetchFirst.min == glm::ivec3(0) && fetchFirst.max == glm::ivec3(34), "default: the first chunk's fetch region is clipped to the bounds");

    const vke::VolumeRegion fetchLast = grid.fetchRegion(63);
    check(fetchLast.min == glm::ivec3(95) && fetchLast.max == glm::ivec3(129), "default: the last chunk's fetch region ends at the bounds");

    check(touched(grid, sampleRegion({ -1, 40, 40 })).empty(), "default: a sample one before the bounds touches nothing");
    check(touched(grid, sampleRegion({ 129, 40, 40 })).empty(), "default: a sample one past the bounds touches nothing");
    check(touched(grid, sampleRegion({ 0, 40, 40 })) == Chunks({ 20 }), "default: the first sample touches the first chunk of its row");
    check(touched(grid, sampleRegion({ 128, 40, 40 })) == Chunks({ 23 }), "default: the last sample touches the last chunk of its row");
  }

  void checkSampleBoundsFetchRegions()
  {
    const vke::VolumeChunkGrid grid({ { 0, 0, 0 }, { 129, 129, 129 } }, { { -1, 0, -5 }, { 130, 129, 140 } }, 32);

    const vke::VolumeRegion first = grid.fetchRegion(0);
    check(first.min == glm::ivec3(-1, 0, -1) && first.max == glm::ivec3(34, 34, 34), "fetch regions reach into wider sample bounds");

    const vke::VolumeRegion last = grid.fetchRegion(63);
    check(last.min == glm::ivec3(95) && last.max == glm::ivec3(130, 129, 130), "the last chunk fetches up to the sample bounds' maximum");

    check(grid.getBounds().min == glm::ivec3(0) && grid.getBounds().max == glm::ivec3(129), "wider sample bounds leave the bounds alone");
    check(grid.chunkCount() == 64, "wider sample bounds leave the chunks alone");

    const vke::VolumeChunkGrid narrow({ { 0, 0, 0 }, { 129, 129, 129 } }, 32);
    check(grid.chunkCount() == narrow.chunkCount() && grid.chunkCounts() == narrow.chunkCounts(),
          "wider sample bounds give the chunk counts of the default grid");

    const vke::CellRange firstCells = grid.cellRange(0);
    const vke::CellRange lastCells = grid.cellRange(grid.chunkCount() - 1);
    check(firstCells.min == grid.getBounds().min, "the first chunk's cells start at the bounds, not the sample bounds");
    check(lastCells.max == grid.getBounds().max - 1, "the last chunk's cells end at the bounds, not the sample bounds");

    bool sameRanges = true;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      sameRanges = sameRanges && grid.cellRange(chunk).min == narrow.cellRange(chunk).min &&
                   grid.cellRange(chunk).max == narrow.cellRange(chunk).max;
    }

    check(sameRanges, "every chunk's cell range is that of the default grid");
  }

  // The chunks whose fetch region contains the sample
  Chunks chunksFetching(const vke::VolumeChunkGrid& grid,
                        const glm::ivec3 position)
  {
    Chunks expected;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      if (!vke::isEmpty(vke::intersect(grid.fetchRegion(chunk), sampleRegion(position))))
      {
        expected.push_back(chunk);
      }
    }

    return expected;
  }

  void checkTouchedChunksOutsideBounds()
  {
    const vke::VolumeRegion bounds { { 0, 0, 0 }, { 129, 129, 129 } };
    const vke::VolumeChunkGrid grid(bounds, { { -5, -5, -5 }, { 134, 134, 134 } }, 32);

    // The chunks of the row at y = z = 40 are 20 to 23 along x
    check(touched(grid, sampleRegion({ -1, 40, 40 })) == Chunks({ 20 }), "a sample one before the bounds touches the edge chunk");
    check(touched(grid, sampleRegion({ 129, 40, 40 })) == Chunks({ 23 }), "a sample one past the bounds touches the edge chunk");
    check(touched(grid, sampleRegion({ -1, 40, 40 })) == chunksFetching(grid, { -1, 40, 40 }), "the chunks touched on the min side are those fetching the sample");
    check(touched(grid, sampleRegion({ 129, 40, 40 })) == chunksFetching(grid, { 129, 40, 40 }), "the chunks touched on the max side are those fetching the sample");
    check(touched(grid, sampleRegion({ -1, -1, -1 })) == Chunks({ 0 }), "a sample diagonally outside the corner touches the corner chunk");
    check(touched(grid, sampleRegion({ 129, 129, 129 })) == Chunks({ 63 }), "a sample diagonally outside the far corner touches the far chunk");
    check(touched(grid, { { -4, 40, 40 }, { 3, 41, 41 } }) == Chunks({ 20 }), "a region straddling the min face touches its edge chunk");

    for (const int far : { -2, -3, -5, 130, 131, 133 })
    {
      check(touched(grid, sampleRegion({ far, 40, 40 })).empty(), "a sample " + std::to_string(far) + " is more than one past the bounds and touches nothing");
    }

    check(touched(grid, { { -5, 40, 40 }, { -1, 41, 41 } }).empty(), "a region wholly more than one before the bounds touches nothing");
    check(touched(grid, { { 130, 40, 40 }, { 134, 41, 41 } }).empty(), "a region wholly more than one past the bounds touches nothing");

    // Sample bounds wider on one side only
    const vke::VolumeChunkGrid oneSided(bounds, { { -1, 0, 0 }, { 129, 129, 129 } }, 32);
    check(touched(oneSided, sampleRegion({ -1, 40, 40 })) == Chunks({ 20 }), "a sample inside the sample bounds is touched");
    check(touched(oneSided, sampleRegion({ 129, 40, 40 })).empty(), "a sample outside the sample bounds is ignored");
  }

  void checkSampleBoundsContainBounds()
  {
    const vke::VolumeRegion bounds { { 0, 0, 0 }, { 8, 8, 8 } };

    check(!throwsInvalidArgument([&] { vke::VolumeChunkGrid(bounds, bounds, 8); }), "sample bounds equal to the bounds are allowed");
    check(!throwsInvalidArgument([&] { vke::VolumeChunkGrid(bounds, { { -1, -1, -1 }, { 9, 9, 9 } }, 8); }), "wider sample bounds are allowed");

    for (int axis = 0; axis < 3; ++axis)
    {
      vke::VolumeRegion highMin = bounds;
      highMin.min[axis] = 1;
      vke::VolumeRegion lowMax = bounds;
      lowMax.max[axis] = 7;

      check(throwsInvalidArgument([&] { vke::VolumeChunkGrid(bounds, highMin, 8); }), "sample bounds starting inside the bounds throw on axis " + std::to_string(axis));
      check(throwsInvalidArgument([&] { vke::VolumeChunkGrid(bounds, lowMax, 8); }), "sample bounds ending inside the bounds throw on axis " + std::to_string(axis));
    }

    const vke::VolumeRegion empty { { 3, 3, 3 }, { 3, 3, 3 } };
    check(!throwsInvalidArgument([&] { vke::VolumeChunkGrid(empty, empty, 8); }), "empty bounds with equal sample bounds are allowed");
    check(vke::VolumeChunkGrid(empty, { { 2, 2, 2 }, { 4, 4, 4 } }, 8).chunkCount() == 0, "empty bounds with wider sample bounds have no chunks");
    check(throwsInvalidArgument([&] { vke::VolumeChunkGrid(empty, { { 4, 4, 4 }, { 4, 4, 4 } }, 8); }), "sample bounds elsewhere than empty bounds throw");
    check(throwsInvalidArgument([&] { vke::VolumeChunkGrid(bounds, bounds, 0); }), "chunk size 0 still throws with sample bounds");
  }

  vke::VolumeRegion randomRegion(std::mt19937& generator,
                                 const vke::VolumeRegion& around)
  {
    std::uniform_int_distribution<int> start(-12, 12);
    std::uniform_int_distribution<int> length(-1, 40);
    const glm::ivec3 low = around.min + glm::ivec3(start(generator), start(generator), start(generator));

    return { low, low + glm::ivec3(length(generator), length(generator), length(generator)) };
  }

  void checkTouchedChunksAgainstBruteForce()
  {
    const vke::VolumeRegion bounds { { -5, 3, 10 }, { 80, 50, 64 } };
    const vke::VolumeChunkGrid grid(bounds, 7);
    std::mt19937 generator(7);

    size_t mismatches = 0;
    size_t nonEmpty = 0;

    for (int i = 0; i < 400; ++i)
    {
      vke::VolumeRegion region = randomRegion(generator, bounds);

      if (i % 3 == 0)
      {
        // Mostly inside, where the interesting boundaries are
        region = vke::intersect(region, bounds);
      }

      Chunks expected;

      for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
      {
        if (!vke::isEmpty(vke::intersect(grid.neededSamples(chunk), region)) &&
            !vke::isEmpty(vke::intersect(grid.fetchRegion(chunk), region)))
        {
          expected.push_back(chunk);
        }
      }

      mismatches += touched(grid, region) == expected ? 0 : 1;
      nonEmpty += expected.empty() ? 0 : 1;
    }

    check(mismatches == 0, "touchedChunks matches a scan of every chunk's needed samples");
    check(nonEmpty > 100, "the random regions touch chunks");
  }

  Chunks chunksFetchingRegion(const vke::VolumeChunkGrid& grid,
                              const vke::VolumeRegion& region)
  {
    Chunks expected;

    for (uint32_t chunk = 0; chunk < grid.chunkCount(); ++chunk)
    {
      if (!vke::isEmpty(vke::intersect(grid.fetchRegion(chunk), region)))
      {
        expected.push_back(chunk);
      }
    }

    return expected;
  }

  // Random bounds (some empty or one sample thick) with sample bounds 0 to 3 wider per side, and queries reaching up to
  // 8 samples outside. A chunk is touched exactly when its fetch region meets the query.
  void checkTouchedChunksWithSampleBoundsAgainstBruteForce()
  {
    std::mt19937 generator(2024);
    std::uniform_int_distribution<int> origin(-6, 6);
    std::uniform_int_distribution<int> size(0, 30);
    std::uniform_int_distribution<int> thin(0, 2);
    std::uniform_int_distribution<int> margin(0, 3);
    std::uniform_int_distribution<int> chunkCells(1, 9);
    std::uniform_int_distribution<int> reach(-8, 8);
    std::uniform_int_distribution<int> length(-1, 20);

    size_t mismatches = 0;
    size_t nonEmpty = 0;
    size_t cases = 0;

    for (int i = 0; i < 4000; ++i)
    {
      vke::VolumeRegion bounds;
      vke::VolumeRegion sampleBounds;

      for (int axis = 0; axis < 3; ++axis)
      {
        bounds.min[axis] = origin(generator);
        bounds.max[axis] = bounds.min[axis] + (i % 5 == 0 ? thin(generator) : size(generator));
        sampleBounds.min[axis] = bounds.min[axis] - margin(generator);
        sampleBounds.max[axis] = bounds.max[axis] + margin(generator);
      }

      const vke::VolumeChunkGrid grid(bounds, sampleBounds, static_cast<uint32_t>(chunkCells(generator)));

      for (int query = 0; query < 3; ++query)
      {
        glm::ivec3 low;
        glm::ivec3 high;

        for (int axis = 0; axis < 3; ++axis)
        {
          low[axis] = (query == 2 ? sampleBounds.min[axis] : bounds.min[axis]) + reach(generator);
          high[axis] = low[axis] + length(generator);
        }

        const vke::VolumeRegion region { low, high };
        const Chunks expected = chunksFetchingRegion(grid, region);

        mismatches += touched(grid, region) == expected ? 0 : 1;
        nonEmpty += expected.empty() ? 0 : 1;
        ++cases;
      }
    }

    check(mismatches == 0, "touchedChunks with sample bounds matches the chunks whose fetch region meets the query");
    check(nonEmpty > cases / 4, "the random queries touch chunks");
  }

  // Scheduler

  using Scheduler = vke::VolumeScheduler<int>;
  using Phase = vke::ChunkJob::Phase;

  // A scheduler with the frame thread's side of a surface: meshes are ints, and what chunks show is recorded.
  struct Harness {
    explicit Harness(const uint32_t chunks)
      : scheduler(chunks)
    {
    }

    Scheduler scheduler;
    std::set<int> unready;
    std::map<uint32_t, int> shown;
    // Chunk and mesh in install order, -1 for a cleared chunk
    std::vector<std::pair<uint32_t, int>> installs;

    Scheduler::JobList invalidate(const Chunks& chunks)
    {
      Scheduler::JobList jobs;
      scheduler.invalidate(chunks, jobs);

      return jobs;
    }

    // As a worker would: finished first, then the result
    Scheduler::JobList finish(const std::shared_ptr<vke::ChunkJob>& job,
                              const std::optional<int> mesh,
                              const bool failed = false)
    {
      job->setPhase(Phase::finished);

      Scheduler::JobList jobs;
      scheduler.onResult(job, mesh, failed, jobs);

      return jobs;
    }

    size_t swap()
    {
      return scheduler.collectSwaps(
        [this](const int& mesh) {
          return !unready.contains(mesh);
        },
        [this](const uint32_t chunk, std::optional<int>&& mesh) {
          installs.emplace_back(chunk, mesh ? *mesh : -1);

          if (mesh)
          {
            shown[chunk] = *mesh;
          }
          else
          {
            shown.erase(chunk);
          }
        });
    }

    // The creation batch of every chunk, finished with mesh ids 100 + chunk and swapped in
    void createAll(const uint32_t chunks)
    {
      Chunks all;

      for (uint32_t chunk = 0; chunk < chunks; ++chunk)
      {
        all.push_back(chunk);
      }

      for (const auto& job : invalidate(all))
      {
        finish(job, 100 + static_cast<int>(job->chunk));
      }

      swap();
      installs.clear();
    }
  };

  void checkCreationBatch()
  {
    Harness h(3);

    const auto jobs = h.invalidate({ 0, 1, 2 });
    check(jobs.size() == 3 && jobs[0]->chunk == 0 && jobs[2]->chunk == 2, "creation submits a job per chunk");
    check(jobs[0]->generation == 1 && jobs[1]->generation == 1, "a batch's jobs share its generation");
    check(h.scheduler.pendingFraction() == 1.0f, "every chunk is pending after creation");

    h.finish(jobs[0], 10);
    h.finish(jobs[1], 11);
    check(h.swap() == 0 && h.shown.empty(), "a batch with a chunk still extracting does not swap");

    h.unready.insert(12);
    h.finish(jobs[2], 12);
    check(h.swap() == 0 && h.shown.empty(), "a batch with an unready mesh does not swap, even for the others");
    check(h.scheduler.pendingFraction() == 1.0f, "an unswapped batch's chunks stay pending");

    h.unready.clear();
    check(h.swap() == 1, "the batch swaps once every mesh is ready");
    check(h.shown.size() == 3 && h.shown[0] == 10 && h.shown[1] == 11 && h.shown[2] == 12, "every chunk shows its mesh");
    check(h.scheduler.pendingFraction() == 0.0f && h.scheduler.openBatchCount() == 0, "nothing is pending after the swap");
  }

  void checkUnreadyMeshKeepsOldMeshes()
  {
    Harness h(3);
    h.createAll(3);

    const auto jobs = h.invalidate({ 0, 1 });
    h.unready.insert(21);
    h.finish(jobs[0], 20);
    h.finish(jobs[1], 21);

    check(h.swap() == 0 && h.installs.empty(), "an unready replacement keeps every old mesh");
    check(h.shown[0] == 100 && h.shown[1] == 101, "the old meshes are still shown");

    h.unready.clear();
    check(h.swap() == 1 && h.shown[0] == 20 && h.shown[1] == 21 && h.shown[2] == 102, "the replacements swap in together");
  }

  void checkQueuedJobIsShared()
  {
    Harness h(3);
    h.createAll(3);

    const auto first = h.invalidate({ 0 });
    check(first.size() == 1, "an edit submits a job");

    const auto second = h.invalidate({ 0, 1 });
    check(second.size() == 1 && second[0]->chunk == 1, "a queued job is not submitted again");
    check(h.scheduler.openBatchCount() == 1, "the new batch absorbed the old one");

    h.finish(first[0], 30);
    check(h.swap() == 0, "the absorbed batch waits for the new job too");

    h.finish(second[0], 31);
    check(h.swap() == 1 && h.shown[0] == 30 && h.shown[1] == 31, "both swap together, as one batch");
  }

  void checkStartedJobGetsFollowUp()
  {
    Harness h(3);
    h.createAll(3);

    const auto first = h.invalidate({ 0 });
    first[0]->setPhase(Phase::started);

    check(h.invalidate({ 0 }).empty(), "an edit during extraction submits nothing yet");
    check(h.invalidate({ 0 }).empty(), "a second edit shares the same follow-up");
    check(h.scheduler.wantsResult(first[0]), "the started job's result is still wanted");

    const auto followUp = h.finish(first[0], 40);
    check(followUp.size() == 1 && followUp[0]->chunk == 0, "its result submits one follow-up extraction");
    check(followUp[0]->generation > first[0]->generation, "in a newer batch");
    check(h.swap() == 1 && h.shown[0] == 40, "the started job's mesh shows while the follow-up runs");

    check(h.finish(followUp[0], 41).empty(), "a follow-up with no edit during it submits nothing");
    check(h.swap() == 1 && h.shown[0] == 41, "the follow-up's mesh replaces it");
  }

  void checkContinuousEditsKeepShowing()
  {
    Harness h(2);
    h.createAll(2);

    auto running = h.invalidate({ 0 });

    for (int step = 0; step < 4; ++step)
    {
      running[0]->setPhase(Phase::started);
      check(h.invalidate({ 0 }).empty(), "an edit during extraction waits for it");

      const auto next = h.finish(running[0], 50 + step);
      check(next.size() == 1, "one extraction at a time, the next submitted when the last finishes");
      check(h.swap() == 1 && h.shown[0] == 50 + step, "a finished extraction shows without waiting for the edits to pause");

      running = next;
    }
  }

  void checkFinishedJobGetsIndependentBatch()
  {
    Harness h(2);

    const auto creation = h.invalidate({ 0, 1 });
    h.finish(creation[0], 10);

    // Chunk 0's job finished and delivered, chunk 1's is still extracting: the creation batch stays open
    const auto edit = h.invalidate({ 0 });
    check(edit.size() == 1 && edit[0]->chunk == 0, "an edit after a finished job submits a new job");
    check(h.scheduler.openBatchCount() == 2, "in a batch of its own");

    h.finish(edit[0], 11);
    check(h.swap() == 0 && h.installs.empty(), "the newer batch waits for the older open one, though it is ready");

    h.finish(creation[1], 12);
    check(h.swap() == 2, "both batches swap, one after the other");
    check(h.installs == std::vector<std::pair<uint32_t, int>>({ { 0, 10 }, { 1, 12 }, { 0, 11 } }), "in generation order");
    check(h.shown[0] == 11 && h.shown[1] == 12, "the newer data ends up shown");
  }

  void checkFollowUpEditsOpenNoEmptyBatch()
  {
    Harness h(2);
    h.createAll(2);

    const auto first = h.invalidate({ 0 });
    first[0]->setPhase(Phase::started);

    check(h.invalidate({ 0 }).empty() && h.scheduler.openBatchCount() == 1, "an edit that only marks a follow-up opens no batch");

    const auto followUp = h.finish(first[0], 70);
    check(followUp.size() == 1, "the follow-up is submitted");

    check(h.invalidate({ 0 }).empty(), "an edit while the follow-up is queued shares it");
    check(h.scheduler.openBatchCount() == 2, "the follow-up's batch absorbed that edit; nothing else opened");

    check(h.finish(followUp[0], 71).empty(), "a queued follow-up reads the edit, so no further job");
    check(h.swap() == 2 && h.shown[0] == 71, "both batches swap and the follow-up's mesh shows");
  }

  void checkCancelWithFollowUpPending()
  {
    Harness h(2);
    h.createAll(2);

    const auto first = h.invalidate({ 0 });
    first[0]->setPhase(Phase::started);
    check(h.invalidate({ 0 }).empty(), "the edit marks a follow-up");

    h.scheduler.cancelAll();

    check(h.finish(first[0], 80).empty(), "a result after cancelAll is dropped and submits no follow-up");
    check(h.swap() == 0 && h.installs.empty(), "nothing swaps after cancelAll");
    check(h.scheduler.pendingFraction() == 0.0f && h.scheduler.openBatchCount() == 0, "nothing stays open");
  }

  void checkFinishedUndeliveredJobGetsFollowUp()
  {
    Harness h(2);
    h.createAll(2);

    const auto first = h.invalidate({ 0 });
    first[0]->setPhase(Phase::finished);

    check(h.invalidate({ 0 }).empty(), "a job finished but not yet delivered is followed up, not doubled");

    const auto followUp = h.finish(first[0], 60);
    check(followUp.size() == 1, "its delivery submits the follow-up");

    h.finish(followUp[0], 61);
    check(h.swap() == 2 && h.shown[0] == 61, "both deliver; the newer shows last");
  }

  void checkOlderEntryDoesNotOverwriteNewer()
  {
    Harness h(2);

    // Chunk 0 is the edited one, chunk 1 the one whose job is still queued
    const auto creation = h.invalidate({ 0, 1 });
    h.finish(creation[0], 50);

    const auto edit = h.invalidate({ 0 });
    check(edit.size() == 1, "chunk 0 is extracted again in its own batch");

    const auto absorbing = h.invalidate({ 1 });
    check(absorbing.empty() && h.scheduler.openBatchCount() == 1, "chunk 1's queued job is shared; both older batches moved to the newest");

    h.finish(edit[0], 51);
    check(h.swap() == 0, "nothing swaps while chunk 1's older data is missing");

    h.finish(creation[1], 52);
    check(h.swap() == 1, "then everything swaps as one batch");
    check(h.shown[1] == 52 && h.shown[0] == 51, "chunk 0 ends on its newer mesh");

    const auto first = std::ranges::find_if(h.installs, [](const auto& install) { return install.second == 50; });
    const auto second = std::ranges::find_if(h.installs, [](const auto& install) { return install.second == 51; });
    check(first < second, "the older mesh was installed before the newer one, never after");
  }

  void checkAbsorbKeepsLaterBatchesBehind()
  {
    Harness h(2);

    const auto edit1 = h.invalidate({ 0, 1 });
    h.finish(edit1[0], 60);

    const auto edit2 = h.invalidate({ 0 });
    check(edit2.size() == 1 && h.scheduler.openBatchCount() == 2, "edit 2 opens its own batch");

    const auto edit3 = h.invalidate({ 1 });
    check(edit3.empty(), "edit 3 shares chunk 1's queued job");

    h.finish(edit2[0], 61);
    check(h.swap() == 0 && h.shown.empty(), "edit 2 does not swap ahead of chunk 1's edit 1 result");

    h.finish(edit1[1], 62);
    check(h.swap() == 1 && h.shown[0] == 61 && h.shown[1] == 62, "both chunks swap together");
  }

  void checkEmptyAndFailedResults()
  {
    Harness h(2);
    h.createAll(2);

    const auto empty = h.invalidate({ 0 });
    h.finish(empty[0], std::nullopt);
    check(h.swap() == 1 && h.shown.count(0) == 0 && h.shown.count(1) == 1, "an empty result clears the chunk");
    check(h.installs.size() == 1 && h.installs[0] == std::make_pair(0u, -1), "as an install of nothing");

    const auto failed = h.invalidate({ 1 });
    h.finish(failed[0], std::nullopt, true);
    h.installs.clear();
    check(h.swap() == 1 && h.installs.empty() && h.shown[1] == 101, "a failed result keeps what the chunk shows");
    check(h.scheduler.pendingFraction() == 0.0f, "and counts as done");

    // A failed job in a batch with a good one: the good one still installs
    const auto both = h.invalidate({ 0, 1 });
    h.finish(both[0], 70);
    h.finish(both[1], std::nullopt, true);
    check(h.swap() == 1 && h.shown[0] == 70 && h.shown[1] == 101, "a failure does not hold back the rest of its batch");
  }

  void checkPendingFraction()
  {
    check(Harness(0).scheduler.pendingFraction() == 0.0f, "no chunks, no pending fraction");

    Harness h(4);
    h.createAll(4);
    check(h.scheduler.pendingFraction() == 0.0f, "nothing pending when everything shows");

    const auto one = h.invalidate({ 1 });
    check(h.scheduler.pendingFraction() == 0.25f, "one chunk of four pending");

    h.finish(one[0], 80);
    h.invalidate({ 1, 2 });
    check(h.scheduler.pendingFraction() == 0.5f, "a chunk in two batches counts once");

    check(h.invalidate({}).empty() && h.scheduler.openBatchCount() == 2, "an empty invalidation opens no batch");
  }

  void checkCancelAll()
  {
    Harness h(3);
    h.createAll(3);

    const auto jobs = h.invalidate({ 0, 1 });
    h.scheduler.cancelAll();

    check(h.scheduler.pendingFraction() == 0.0f && h.scheduler.openBatchCount() == 0, "cancelAll leaves nothing open");
    check(!h.scheduler.wantsResult(jobs[0]), "results of cancelled jobs are unwanted");

    check(h.finish(jobs[0], 90).empty() && h.swap() == 0 && h.installs.empty(), "results of cancelled jobs are ignored");
    check(h.shown.size() == 3, "chunks keep showing what they show");
  }

  void checkUnknownJobIgnored()
  {
    Harness h(1);
    h.createAll(1);

    const auto stranger = std::make_shared<vke::ChunkJob>();
    check(h.finish(stranger, 5).empty() && h.swap() == 0, "a result for a job nobody scheduled is ignored");
  }

} // namespace

int main()
{
  checkGridCounts();
  checkCellRangesAndFetchRegions();
  checkTouchedChunksSingleSample();
  checkDefaultSampleBounds();
  checkSampleBoundsFetchRegions();
  checkTouchedChunksOutsideBounds();
  checkSampleBoundsContainBounds();
  checkTouchedChunksRegions();
  checkTouchedChunksAgainstBruteForce();
  checkTouchedChunksWithSampleBoundsAgainstBruteForce();

  checkCreationBatch();
  checkUnreadyMeshKeepsOldMeshes();
  checkQueuedJobIsShared();
  checkStartedJobGetsFollowUp();
  checkContinuousEditsKeepShowing();
  checkFinishedJobGetsIndependentBatch();
  checkFinishedUndeliveredJobGetsFollowUp();
  checkFollowUpEditsOpenNoEmptyBatch();
  checkCancelWithFollowUpPending();
  checkOlderEntryDoesNotOverwriteNewer();
  checkAbsorbKeepsLaterBatchesBehind();
  checkEmptyAndFailedResults();
  checkPendingFraction();
  checkCancelAll();
  checkUnknownJobIgnored();

  std::cout << g_checks << " checks, " << g_failures << " failures" << std::endl;

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
