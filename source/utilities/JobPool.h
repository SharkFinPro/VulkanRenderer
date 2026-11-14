#ifndef VKE_JOBPOOL_H
#define VKE_JOBPOOL_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace vke {

  // A move-only callable taking no arguments. std::function needs copyable callables and std::move_only_function is
  // not available in every supported standard library.
  class Job {
  public:
    Job() = default;

    template <typename F>
      requires(!std::is_same_v<std::decay_t<F>, Job> && std::is_move_constructible_v<std::decay_t<F>> &&
               std::is_invocable_r_v<void, std::decay_t<F>&>)
    Job(F&& callable)
      : m_callable(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(callable)))
    {
    }

    Job(Job&&) noexcept = default;
    Job& operator=(Job&&) noexcept = default;

    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    // Must not be called on an empty job.
    void operator()()
    {
      m_callable->invoke();
    }

    explicit operator bool() const
    {
      return m_callable != nullptr;
    }

  private:
    struct Base {
      virtual ~Base() = default;
      virtual void invoke() = 0;
    };

    template <typename F>
    struct Model final : Base {
      template <typename U>
      explicit Model(U&& callable)
        : m_callable(std::forward<U>(callable))
      {
      }

      void invoke() override
      {
        m_callable();
      }

      F m_callable;
    };

    std::unique_ptr<Base> m_callable;
  };

  // Shared flag for work that became obsolete (say, a region that changed again). Copies refer to the same flag.
  class CancellationToken {
  public:
    CancellationToken()
      : m_cancelled(std::make_shared<std::atomic<bool>>(false))
    {
    }

    // Thread-safe and idempotent.
    void cancel() const
    {
      m_cancelled->store(true, std::memory_order_release);
    }

    [[nodiscard]] bool isCancelled() const
    {
      return m_cancelled->load(std::memory_order_acquire);
    }

  private:
    std::shared_ptr<std::atomic<bool>> m_cancelled;
  };

  // Fixed worker threads draining a priority queue. Workers block while idle. A job that throws is counted and
  // reported, never propagated. Anything a job produces for the GPU must go through a CompletionQueue.
  class JobPool {
  public:
    // 0 = defaultWorkerCount().
    explicit JobPool(uint32_t workerCount = 0);

    // Running jobs finish; queued jobs are destroyed without running.
    ~JobPool();

    JobPool(const JobPool&) = delete;
    JobPool& operator=(const JobPool&) = delete;
    JobPool(JobPool&&) = delete;
    JobPool& operator=(JobPool&&) = delete;

    // Hardware threads minus two (the frame thread and the driver), at least one.
    [[nodiscard]] static uint32_t defaultWorkerCount();

    [[nodiscard]] uint32_t workerCount() const;

    void submit(Job job);

    void submit(Job job,
                int priority);

    // The token is checked right before the job starts; a cancelled job is dropped. A running job is not interrupted
    // (it can capture the token itself).
    void submit(Job job,
                const CancellationToken& token,
                int priority = 0);

    // Queued and not yet started.
    [[nodiscard]] size_t pendingCount() const;

    [[nodiscard]] uint64_t failedCount() const;

    [[nodiscard]] uint64_t cancelledCount() const;

  private:
    struct Entry {
      Job job;
      CancellationToken token;
      bool hasToken = false;
      int priority = 0;
      uint64_t sequence = 0;
    };

    // Heap order: higher priority first, then lower sequence (submission order).
    [[nodiscard]] static bool runsAfter(const Entry& a,
                                        const Entry& b);

    mutable std::mutex m_mutex;
    std::condition_variable m_wakeup;
    std::vector<Entry> m_queue;
    uint64_t m_nextSequence = 0;
    bool m_stopRequested = false;

    std::atomic<uint64_t> m_failedCount{ 0 };
    std::atomic<uint64_t> m_cancelledCount{ 0 };

    std::vector<std::thread> m_workers;

    void enqueue(Entry entry);

    void workerLoop();
  };

} // namespace vke

#endif //VKE_JOBPOOL_H
