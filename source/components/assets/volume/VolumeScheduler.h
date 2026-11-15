#ifndef VKE_VOLUMESCHEDULER_H
#define VKE_VOLUMESCHEDULER_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vke {

  // One chunk extraction, shared between the scheduler (frame thread) and the worker running it.
  struct ChunkJob {
    enum class Phase : uint8_t {
      queued,
      started,
      finished
    };

    uint32_t chunk = 0;
    uint64_t generation = 0;

    // The worker sets started right before it reads the samples and finished before it pushes its result; the scheduler
    // reads with acquire.
    void setPhase(const Phase value)
    {
      m_phase.store(static_cast<uint8_t>(value), std::memory_order_release);
    }

    [[nodiscard]] Phase getPhase() const
    {
      return static_cast<Phase>(m_phase.load(std::memory_order_acquire));
    }

  private:
    std::atomic<uint8_t> m_phase { static_cast<uint8_t>(Phase::queued) };
  };

  // The batch bookkeeping of a volume surface, free of Vulkan and threads so it can be tested alone. MeshHandle is
  // whatever a chunk displays (a shared_ptr to a mesh); the scheduler only stores and moves it. Frame thread only; the
  // workers touch nothing but their ChunkJob's phase.
  //
  // A batch is everything one invalidate call scheduled (creation touches every chunk) and gets a generation above all
  // earlier ones. A batch swaps in, every chunk replacing its displayed mesh in the same call, once every job in it has
  // a final result and every mesh is ready. Batches swap in generation order, so a finished batch waits for older open
  // ones.
  //
  // An edit of a chunk whose latest job has not read the samples yet needs no new job: the queued job reads post-edit
  // data, so the new batch absorbs the old one. A job that has started may have read pre-edit data: it is absorbed too,
  // but its result is discarded and the chunk extracted again. A finished job gets a new job in an independent batch.
  // Entries remember the generation of the job that produced their data, and a chunk never shows older data than it
  // already shows. Absorbing also takes every batch opened after the absorbed one, so no batch swaps ahead of an
  // absorbed entry.
  //
  // Known limit: when extracting a chunk keeps taking longer than the gap between edits of it, each edit absorbs the
  // open batch, which therefore stays open until the edits pause.
  template <typename MeshHandle>
  class VolumeScheduler {
  public:
    using JobList = std::vector<std::shared_ptr<ChunkJob>>;

    explicit VolumeScheduler(const uint32_t chunkCount)
      : m_chunks(chunkCount)
    {
    }

    // Opens a batch for the chunks and appends the jobs to run to newJobs, which the caller submits. Chunks must be
    // distinct and below the chunk count.
    void invalidate(const std::span<const uint32_t> chunks,
                    JobList& newJobs)
    {
      if (chunks.empty())
      {
        return;
      }

      // Pairs with the fence in the worker: either the phase read below says started, or the worker sees the edit
      std::atomic_thread_fence(std::memory_order_seq_cst);

      m_batches.emplace_back();

      Batch* batch = &m_batches.back();
      batch->generation = ++m_lastGeneration;

      for (const uint32_t chunk : chunks)
      {
        invalidateChunk(chunk, *batch, newJobs);
      }
    }

    // The result of a job: no mesh means the chunk has no surface, failed means extraction threw. A result of a job that
    // is not (or no longer) current is dropped. May append the re-extraction of a chunk edited while its job ran.
    void onResult(const std::shared_ptr<ChunkJob>& job,
                  std::optional<MeshHandle> mesh,
                  const bool failed,
                  JobList& newJobs)
    {
      const auto found = m_entries.find(job.get());

      if (found == m_entries.end())
      {
        return;
      }

      Entry& entry = *found->second;

      if (entry.reExtract)
      {
        restart(entry, newJobs);
        return;
      }

      entry.done = true;
      entry.failed = failed;
      entry.mesh = std::move(mesh);

      --entry.batch->pending;
    }

    // Whether onResult would keep this job's result, so a caller can skip building what would be discarded.
    [[nodiscard]] bool wantsResult(const std::shared_ptr<ChunkJob>& job) const
    {
      const auto found = m_entries.find(job.get());

      return found != m_entries.end() && !found->second->reExtract;
    }

    // Swaps in every leading batch that is complete and whose meshes are ready: calls install(chunk,
    // std::optional<MeshHandle>&&) for each entry newer than what the chunk shows (an empty optional clears it).
    // Returns how many batches swapped.
    template <typename IsReady, typename Install>
    size_t collectSwaps(IsReady&& isReady,
                        Install&& install)
    {
      size_t swapped = 0;

      while (!m_batches.empty() && batchIsReady(m_batches.front(), isReady))
      {
        swapBatch(m_batches.front(), install);

        m_batches.pop_front();
        ++swapped;
      }

      return swapped;
    }

    // Chunks in any open batch over all chunks; 0 with no chunks.
    [[nodiscard]] float pendingFraction() const
    {
      if (m_chunks.empty())
      {
        return 0.0f;
      }

      return static_cast<float>(m_pendingChunks) / static_cast<float>(m_chunks.size());
    }

    [[nodiscard]] size_t openBatchCount() const
    {
      return m_batches.size();
    }

    // Forgets every open batch and job, so later results are dropped. What chunks show stays.
    void cancelAll()
    {
      m_batches.clear();
      m_entries.clear();

      for (ChunkState& chunk : m_chunks)
      {
        chunk.latest = nullptr;
        chunk.openEntries = 0;
      }

      m_pendingChunks = 0;
    }

  private:
    struct Batch;

    struct Entry {
      Batch* batch = nullptr;
      std::shared_ptr<ChunkJob> job;
      uint32_t chunk = 0;
      // Of the job that produced the data
      uint64_t generation = 0;
      bool done = false;
      bool failed = false;
      // The job started before an edit: its result is discarded and the chunk extracted again at nextGeneration
      bool reExtract = false;
      uint64_t nextGeneration = 0;
      std::optional<MeshHandle> mesh;
    };

    struct Batch {
      uint64_t generation = 0;
      std::vector<std::unique_ptr<Entry>> entries;
      // Entries without a result yet
      size_t pending = 0;
    };

    struct ChunkState {
      // The entry of the newest job, or null with no open batch holding the chunk
      Entry* latest = nullptr;
      uint32_t openEntries = 0;
      // The generation of the data on display; 0 is none
      uint64_t shownGeneration = 0;
    };

    std::vector<ChunkState> m_chunks;

    // Oldest first; a list, so entries can point at their batch while batches come and go
    std::list<Batch> m_batches;

    std::unordered_map<const ChunkJob*, Entry*> m_entries;

    uint64_t m_lastGeneration = 0;
    size_t m_pendingChunks = 0;

    void invalidateChunk(const uint32_t chunk,
                         Batch& batch,
                         JobList& newJobs)
    {
      Entry* latest = m_chunks[chunk].latest;

      if (latest != nullptr && !latest->done)
      {
        if (latest->reExtract)
        {
          // Its replacement is not submitted yet and will read post-edit data
          latest->nextGeneration = batch.generation;
          absorb(*latest->batch, batch);
          return;
        }

        const ChunkJob::Phase phase = latest->job->getPhase();

        if (phase != ChunkJob::Phase::finished)
        {
          if (phase == ChunkJob::Phase::started)
          {
            latest->reExtract = true;
            latest->nextGeneration = batch.generation;
          }

          absorb(*latest->batch, batch);
          return;
        }
      }

      addEntry(chunk, batch, newJobs);
    }

    // Moves the entries of an open batch and of every batch after it into the newest one, so nothing swaps ahead of an
    // absorbed entry.
    void absorb(Batch& from,
                Batch& into)
    {
      if (&from == &into)
      {
        return;
      }

      auto it = m_batches.begin();

      while (&*it != &from)
      {
        ++it;
      }

      while (&*it != &into)
      {
        moveEntries(*it, into);
        it = m_batches.erase(it);
      }
    }

    static void moveEntries(Batch& from,
                            Batch& into)
    {
      for (auto& entry : from.entries)
      {
        entry->batch = &into;
        into.entries.push_back(std::move(entry));
      }

      into.pending += from.pending;
    }

    void addEntry(const uint32_t chunk,
                  Batch& batch,
                  JobList& newJobs)
    {
      auto job = std::make_shared<ChunkJob>();
      job->chunk = chunk;
      job->generation = batch.generation;

      auto entry = std::make_unique<Entry>();
      entry->batch = &batch;
      entry->job = job;
      entry->chunk = chunk;
      entry->generation = batch.generation;

      ChunkState& state = m_chunks[chunk];
      state.latest = entry.get();

      if (state.openEntries++ == 0)
      {
        ++m_pendingChunks;
      }

      m_entries[job.get()] = entry.get();
      batch.entries.push_back(std::move(entry));
      ++batch.pending;

      newJobs.push_back(std::move(job));
    }

    // Replaces the discarded job of an entry that must be extracted again.
    void restart(Entry& entry,
                 JobList& newJobs)
    {
      m_entries.erase(entry.job.get());

      auto job = std::make_shared<ChunkJob>();
      job->chunk = entry.chunk;
      job->generation = entry.nextGeneration;

      entry.job = job;
      entry.generation = entry.nextGeneration;
      entry.reExtract = false;

      m_entries[job.get()] = &entry;

      newJobs.push_back(std::move(job));
    }

    template <typename IsReady>
    [[nodiscard]] static bool batchIsReady(const Batch& batch,
                                           IsReady& isReady)
    {
      if (batch.pending != 0)
      {
        return false;
      }

      return std::ranges::all_of(batch.entries, [&isReady](const std::unique_ptr<Entry>& entry) {
        return !entry->mesh.has_value() || isReady(*entry->mesh);
      });
    }

    template <typename Install>
    void swapBatch(Batch& batch,
                   Install& install)
    {
      for (auto& entry : batch.entries)
      {
        ChunkState& state = m_chunks[entry->chunk];

        if (!entry->failed && entry->generation > state.shownGeneration)
        {
          state.shownGeneration = entry->generation;
          install(entry->chunk, std::move(entry->mesh));
        }

        if (state.latest == entry.get())
        {
          state.latest = nullptr;
        }

        if (--state.openEntries == 0)
        {
          --m_pendingChunks;
        }

        m_entries.erase(entry->job.get());
      }
    }
  };

} // namespace vke

#endif //VKE_VOLUMESCHEDULER_H
