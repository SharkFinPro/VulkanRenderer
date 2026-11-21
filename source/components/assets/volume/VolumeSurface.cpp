#include "VolumeSurface.h"
#include "MarchingCubesExtractor.h"
#include "../objects/Mesh.h"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

namespace vke {

  namespace {

    const VolumeSource& requireSource(const std::shared_ptr<const VolumeSource>& source)
    {
      if (!source)
      {
        throw std::invalid_argument("A volume surface needs a source.");
      }

      return *source;
    }

    const VolumeSurfaceDescription& validate(const VolumeSurfaceDescription& description)
    {
      if (!std::isfinite(description.cellSize) || description.cellSize <= 0.0f)
      {
        throw std::invalid_argument("A volume surface's cell size must be finite and positive.");
      }

      return description;
    }

    // One chunk extraction as the job pool runs it. It holds shared state only, never the surface, so it may outlive it.
    struct ChunkTask {
      std::shared_ptr<const VolumeSource> source;
      std::shared_ptr<const SurfaceExtractor> extractor;
      std::shared_ptr<CompletionQueue<ChunkResult>> results;
      std::shared_ptr<ChunkJob> job;
      VolumeRegion region;
      CellRange cells;
      float cellSize = 1.0f;

      void operator()() const
      {
        ChunkResult result { job, {}, false };

        try
        {
          extract(result.mesh);
        }
        catch (const std::exception& e)
        {
          std::cerr << "Volume chunk " << job->chunk << " failed: " << e.what() << std::endl;
          result.failed = true;
        }
        catch (...)
        {
          std::cerr << "Volume chunk " << job->chunk << " failed." << std::endl;
          result.failed = true;
        }

        if (result.failed)
        {
          result.mesh.clear();
        }

        // Before the push, so a result in the queue always belongs to a finished job
        job->setPhase(ChunkJob::Phase::finished);

        results->push(std::move(result));
      }

      void extract(SurfaceMesh& mesh) const
      {
        std::vector<float> distance(sampleCount(region));
        std::vector<uint8_t> material(sampleCount(region));

        job->setPhase(ChunkJob::Phase::started);

        // Pairs with the fence in VolumeScheduler::invalidate: either it sees started, or fill sees the edit
        std::atomic_thread_fence(std::memory_order_seq_cst);

        source->fill(region, 0, distance, material);

        extractor->extract({ region, distance, material }, cells, cellSize, mesh);
      }
    };

  } // namespace

  VolumeSurface::VolumeSurface(std::shared_ptr<LogicalDevice> logicalDevice,
                               std::shared_ptr<UploadQueue> uploadQueue,
                               std::weak_ptr<JobPool> jobPool,
                               const VolumeSurfaceDescription& description,
                               std::shared_ptr<const VolumeSource> source)
    : m_logicalDevice(std::move(logicalDevice)),
      m_uploadQueue(std::move(uploadQueue)),
      m_jobPool(std::move(jobPool)),
      m_description(validate(description)),
      m_source(std::move(source)),
      m_extractor(std::make_shared<MarchingCubesExtractor>()),
      m_grid(requireSource(m_source).bounds(), m_source->sampleBounds(), description.chunkCells),
      m_results(std::make_shared<CompletionQueue<ChunkResult>>()),
      m_scheduler(m_grid.chunkCount()),
      m_meshes(m_grid.chunkCount())
  {
    scheduleAll();
  }

  VolumeSurface::~VolumeSurface()
  {
    cancel();
  }

  void VolumeSurface::invalidate(const VolumeRegion& region)
  {
    if (m_cancelled)
    {
      return;
    }

    m_grid.touchedChunks(region, m_touched);

    m_scheduler.invalidate(m_touched, m_newJobs);

    submitNewJobs();
  }

  float VolumeSurface::pendingFraction() const
  {
    return m_scheduler.pendingFraction();
  }

  void VolumeSurface::update()
  {
    if (m_cancelled)
    {
      return;
    }

    m_results->drain([this](ChunkResult& result) {
      m_backlog.push_back(std::move(result));
    });

    handleBacklog();

    swapReadyBatches();

    submitNewJobs();

    if (m_displayedChanged)
    {
      rebuildDisplayed();
    }
  }

  void VolumeSurface::cancel()
  {
    m_cancelled = true;

    m_cancellation.cancel();

    m_scheduler.cancelAll();

    m_newJobs.clear();

    m_backlog.clear();
  }

  const VolumeSurfaceDescription& VolumeSurface::getDescription() const
  {
    return m_description;
  }

  void VolumeSurface::setCastsShadows(const bool castsShadows)
  {
    m_castsShadows = castsShadows;
  }

  bool VolumeSurface::castsShadows() const
  {
    return m_castsShadows;
  }

  std::span<const DisplayedChunk> VolumeSurface::getDisplayedChunks() const
  {
    return m_displayed;
  }

  void VolumeSurface::scheduleAll()
  {
    m_grid.touchedChunks(m_grid.getBounds(), m_touched);

    m_scheduler.invalidate(m_touched, m_newJobs);

    submitNewJobs();
  }

  void VolumeSurface::submitNewJobs()
  {
    const std::shared_ptr<JobPool> pool = m_jobPool.lock();

    if (pool)
    {
      for (const std::shared_ptr<ChunkJob>& job : m_newJobs)
      {
        submitJob(*pool, job);
      }
    }

    m_newJobs.clear();
  }

  void VolumeSurface::submitJob(JobPool& pool,
                                const std::shared_ptr<ChunkJob>& job) const
  {
    ChunkTask task;
    task.source = m_source;
    task.extractor = m_extractor;
    task.results = m_results;
    task.job = job;
    task.region = m_grid.fetchRegion(job->chunk);
    task.cells = m_grid.cellRange(job->chunk);
    task.cellSize = m_description.cellSize;

    pool.submit(Job(std::move(task)), m_cancellation);
  }

  void VolumeSurface::handleBacklog()
  {
    size_t created = 0;

    while (!m_backlog.empty() && created < s_maxMeshesPerUpdate)
    {
      if (handleResult(m_backlog.front()))
      {
        ++created;
      }

      m_backlog.pop_front();
    }
  }

  bool VolumeSurface::handleResult(ChunkResult& result)
  {
    std::optional<std::shared_ptr<Mesh>> mesh;
    bool failed = result.failed;

    // A result about to be discarded is not worth a GPU mesh
    if (!failed && !result.mesh.indices.empty() && m_scheduler.wantsResult(result.job))
    {
      try
      {
        mesh = createMesh(result.mesh);
      }
      catch (const std::exception& e)
      {
        std::cerr << "Volume chunk " << result.job->chunk << " mesh failed: " << e.what() << std::endl;
        failed = true;
      }
    }

    const bool created = mesh.has_value();

    m_scheduler.onResult(result.job, std::move(mesh), failed, m_newJobs);

    return created;
  }

  std::shared_ptr<Mesh> VolumeSurface::createMesh(const SurfaceMesh& mesh) const
  {
    // The mesh copies the bytes at creation, so the scratch is free again when this returns
    m_vertexScratch.clear();

    for (const SurfaceVertex& vertex : mesh.vertices)
    {
      m_vertexScratch.push_back({
        .position = vertex.position,
        .normal = vertex.normal,
        .materials = { static_cast<uint8_t>(vertex.material), 0, 0, 0 },
        .weights = { 255, 0, 0, 0 }
      });
    }

    const MeshCreateInfo createInfo {
      .vertexData = std::as_bytes(std::span<const VolumeVertex>(m_vertexScratch)),
      .vertexStride = sizeof(VolumeVertex),
      .positionOffset = offsetof(VolumeVertex, position),
      .indices = mesh.indices,
      .buildBLAS = false
    };

    return std::make_shared<Mesh>(m_logicalDevice, m_uploadQueue, createInfo);
  }

  void VolumeSurface::swapReadyBatches()
  {
    const size_t swapped = m_scheduler.collectSwaps(
      [](const std::shared_ptr<Mesh>& mesh) {
        return mesh->isReady();
      },
      [this](const uint32_t chunk, std::optional<std::shared_ptr<Mesh>>&& mesh) {
        m_meshes[chunk] = mesh ? std::move(*mesh) : nullptr;
      });

    if (swapped > 0)
    {
      m_displayedChanged = true;
    }
  }

  void VolumeSurface::rebuildDisplayed()
  {
    m_displayed.clear();

    for (uint32_t chunk = 0; chunk < m_meshes.size(); ++chunk)
    {
      if (m_meshes[chunk])
      {
        m_displayed.push_back({ m_meshes[chunk].get(), m_grid.localBounds(chunk, m_description.cellSize) });
      }
    }

    m_displayedChanged = false;
  }

} // namespace vke
