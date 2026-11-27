#ifndef VKE_VOLUMESURFACE_H
#define VKE_VOLUMESURFACE_H

#include "VolumeChunking.h"
#include "VolumeMaterial.h"
#include "VolumePalette.h"
#include "VolumeScheduler.h"
#include "VolumeSource.h"
#include "../../../utilities/Bounds.h"
#include "../../../utilities/CompletionQueue.h"
#include "../../../utilities/JobPool.h"
#include "VolumeVertex.h"
#include <cstdint>
#include <deque>
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
    // Throws std::invalid_argument on a null source, a cell size that is not finite and positive, an invalid chunk size
    // or bounds, or sample bounds that do not contain the bounds. Schedules every chunk. The surface only observes the
    // job pool: without one, nothing is extracted.
    VolumeSurface(std::shared_ptr<LogicalDevice> logicalDevice,
                  std::shared_ptr<UploadQueue> uploadQueue,
                  std::weak_ptr<JobPool> jobPool,
                  const VolumeSurfaceDescription& description,
                  std::shared_ptr<const VolumeSource> source,
                  VolumePaletteContext paletteContext,
                  std::shared_ptr<VolumePalette> defaultPalette);

    // Cancels the jobs that have not started.
    ~VolumeSurface();

    VolumeSurface(const VolumeSurface&) = delete;
    VolumeSurface& operator=(const VolumeSurface&) = delete;

    // Reports that the samples in the region changed (see the thread rule on VolumeSource). The region is clipped to the
    // samples the surface can read (the sample bounds, and no more than one past the bounds), and an empty one does
    // nothing. The chunks it touches are extracted again and swap in together once every one of them is ready, so the
    // surface changes without cracks.
    void invalidate(const VolumeRegion& region);

    // The share of chunks waiting for an edit or the first extraction to show: 0 when the surface shows everything
    // reported so far.
    [[nodiscard]] float pendingFraction() const;

    // Sets the materials that vertex material indices select (at most s_maxVolumeMaterials, see VolumeMaterial), at the
    // cost of building two texture arrays now. The textures are copied into the arrays, so replacing a palette texture
    // means calling this again. An empty palette restores the default (eight flat colors). An index at or above the
    // palette's size wraps (index % size). Nothing is extracted again, and the previous palette is retired, so frames in
    // flight may still draw with it. Throws std::invalid_argument on an invalid palette and then changes nothing. Frame
    // thread only.
    void setPalette(std::span<const VolumeMaterial> palette);

    // The following are called by the engine.

    // Once per frame: takes finished extractions, creates their meshes (at most 16 per call) and swaps in every batch that is complete and
    // uploaded.
    void update();

    // Stops all work; the surface keeps showing what it shows. Done when it is released.
    void cancel();

    [[nodiscard]] const VolumeSurfaceDescription& getDescription() const;

    // Whether the surface is drawn into shadow maps (default true). It is always lit and receives shadows.
    void setCastsShadows(bool castsShadows);

    [[nodiscard]] bool castsShadows() const;

    // The palette to draw with, never null while the surface is alive
    [[nodiscard]] const VolumePalette* getPalette() const;

    // The chunks to draw, in no particular order, valid until the next update().
    [[nodiscard]] std::span<const DisplayedChunk> getDisplayedChunks() const;

  private:
    // Meshes created per update(), so a broad edit spreads over frames; the rest of the results wait their turn
    static constexpr size_t s_maxMeshesPerUpdate = 16;

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

    bool m_castsShadows = true;

    // By chunk; null where a chunk shows nothing
    std::vector<std::shared_ptr<Mesh>> m_meshes;

    VolumePaletteContext m_paletteContext;
    std::shared_ptr<VolumePalette> m_defaultPalette;
    std::shared_ptr<VolumePalette> m_palette;

    // The non-null meshes with their bounds, rebuilt when a batch swaps
    std::vector<DisplayedChunk> m_displayed;
    bool m_displayedChanged = false;

    // Drained results not handled yet, oldest first
    std::deque<ChunkResult> m_backlog;

    // Scratch reused by every call
    std::vector<uint32_t> m_touched;
    mutable std::vector<VolumeVertex> m_vertexScratch;
    Scheduler::JobList m_newJobs;

    void scheduleAll();

    void submitNewJobs();

    void submitJob(JobPool& pool,
                   const std::shared_ptr<ChunkJob>& job) const;

    // Whether a mesh was created
    bool handleResult(ChunkResult& result);

    void handleBacklog();

    [[nodiscard]] std::shared_ptr<Mesh> createMesh(const SurfaceMesh& mesh) const;

    void swapReadyBatches();

    void rebuildDisplayed();
  };

} // namespace vke

#endif //VKE_VOLUMESURFACE_H
