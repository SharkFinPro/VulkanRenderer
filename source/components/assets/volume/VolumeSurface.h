#ifndef VKE_VOLUMESURFACE_H
#define VKE_VOLUMESURFACE_H

#include "VolumeChunking.h"
#include "VolumeScheduler.h"
#include "VolumeSource.h"
#include "../../../utilities/Bounds.h"
#include "../../../utilities/CompletionQueue.h"
#include "../../../utilities/JobPool.h"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace vke {

  class LogicalDevice;
  class Mesh;
  class SurfaceExtractor;
  class UploadQueue;

  // What a worker hands back for one chunk job.
  struct ChunkResult {
    std::shared_ptr<ChunkJob> job;
    SurfaceMesh mesh;
    bool failed = false;
  };

  // A chunk's mesh as the renderer draws it. The mesh belongs to the surface and stays valid until its next update().
  struct DisplayedChunk {
    const Mesh* mesh = nullptr;
    // In the surface's local space
    AABB localBounds;
  };

  // The zero surface of a signed distance field the application owns (see VolumeSource), drawn by the engine. The
  // engine chunks the volume, extracts each chunk on the job pool, uploads the meshes and swaps edits in a batch at a
  // time, so the application never sees a mesh. Create one with AssetManager::createVolumeSurface and release it with
  // AssetManager::release. Frame thread only.
  class VolumeSurface {
  public:
    // Throws std::invalid_argument on a null source, a cell size that is not finite and positive, or invalid chunk size
    // or bounds. Schedules every chunk. The surface only observes the job pool: without one, nothing is extracted.
    VolumeSurface(std::shared_ptr<LogicalDevice> logicalDevice,
                  std::shared_ptr<UploadQueue> uploadQueue,
                  std::weak_ptr<JobPool> jobPool,
                  const VolumeSurfaceDescription& description,
                  std::shared_ptr<const VolumeSource> source);

    // Cancels the jobs that have not started.
    ~VolumeSurface();

    VolumeSurface(const VolumeSurface&) = delete;
    VolumeSurface& operator=(const VolumeSurface&) = delete;

    // Reports that the samples in the region changed (see the thread rule on VolumeSource). The region is clipped to the
    // bounds, and an empty one does nothing. The chunks it touches are extracted again and swap in together once
    // every one of them is ready, so the surface changes without cracks.
    void invalidate(const VolumeRegion& region);

    // The share of chunks waiting for an edit or the first extraction to show: 0 when the surface shows everything
    // reported so far.
    [[nodiscard]] float pendingFraction() const;

    // The following are called by the engine.

    // Once per frame: takes finished extractions, creates their meshes and swaps in every batch that is complete and
    // uploaded.
    void update();

    // Stops all work; the surface keeps showing what it shows. Done when it is released.
    void cancel();

    [[nodiscard]] const VolumeSurfaceDescription& getDescription() const;

    // The chunks to draw, in no particular order, valid until the next update().
    [[nodiscard]] std::span<const DisplayedChunk> getDisplayedChunks() const;

  private:
    using Scheduler = VolumeScheduler<std::shared_ptr<Mesh>>;

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<UploadQueue> m_uploadQueue;

    std::weak_ptr<JobPool> m_jobPool;

    VolumeSurfaceDescription m_description;

    std::shared_ptr<const VolumeSource> m_source;

    std::shared_ptr<const SurfaceExtractor> m_extractor;

    VolumeChunkGrid m_grid;

    CancellationToken m_cancellation;

    std::shared_ptr<CompletionQueue<ChunkResult>> m_results;

    Scheduler m_scheduler;

    bool m_cancelled = false;

    // By chunk; null where a chunk shows nothing
    std::vector<std::shared_ptr<Mesh>> m_meshes;

    // The non-null meshes with their bounds, rebuilt when a batch swaps
    std::vector<DisplayedChunk> m_displayed;
    bool m_displayedChanged = false;

    // Scratch reused by every call
    std::vector<uint32_t> m_touched;
    Scheduler::JobList m_newJobs;

    void scheduleAll();

    void submitNewJobs();

    void submitJob(JobPool& pool,
                   const std::shared_ptr<ChunkJob>& job) const;

    void handleResult(ChunkResult& result);

    [[nodiscard]] std::shared_ptr<Mesh> createMesh(const SurfaceMesh& mesh) const;

    void swapReadyBatches();

    void rebuildDisplayed();
  };

} // namespace vke

#endif //VKE_VOLUMESURFACE_H
