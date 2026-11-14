#include "JobPool.h"

#include <algorithm>
#include <exception>
#include <iostream>

namespace vke {

  JobPool::JobPool(const uint32_t workerCount)
  {
    const uint32_t count = workerCount == 0 ? defaultWorkerCount() : workerCount;

    m_workers.reserve(count);

    try
    {
      for (uint32_t i = 0; i < count; ++i)
      {
        m_workers.emplace_back([this] { workerLoop(); });
      }
    }
    catch (...)
    {
      // The destructor does not run for a constructor that throws
      {
        const std::lock_guard lock(m_mutex);
        m_stopRequested = true;
      }

      m_wakeup.notify_all();

      for (std::thread& worker : m_workers)
      {
        worker.join();
      }

      throw;
    }
  }

  JobPool::~JobPool()
  {
    {
      const std::lock_guard lock(m_mutex);
      m_stopRequested = true;
    }

    m_wakeup.notify_all();

    for (std::thread& worker : m_workers)
    {
      worker.join();
    }

    // Destroyed outside the member, so a captured destructor that submits doesn't modify the vector being cleared.
    std::vector<Entry> discarded = std::move(m_queue);
    discarded.clear();
  }

  uint32_t JobPool::defaultWorkerCount()
  {
    const uint32_t hardwareThreads = std::thread::hardware_concurrency();

    return hardwareThreads > 2 ? hardwareThreads - 2 : 1;
  }

  uint32_t JobPool::workerCount() const
  {
    return static_cast<uint32_t>(m_workers.size());
  }

  void JobPool::submit(Job job)
  {
    submit(std::move(job), 0);
  }

  void JobPool::submit(Job job,
                       const int priority)
  {
    Entry entry;
    entry.job = std::move(job);
    entry.priority = priority;

    enqueue(std::move(entry));
  }

  void JobPool::submit(Job job,
                       const CancellationToken& token,
                       const int priority)
  {
    Entry entry;
    entry.job = std::move(job);
    entry.token = token;
    entry.priority = priority;

    enqueue(std::move(entry));
  }

  size_t JobPool::pendingCount() const
  {
    const std::lock_guard lock(m_mutex);

    return m_queue.size();
  }

  uint64_t JobPool::failedCount() const
  {
    return m_failedCount.load();
  }

  uint64_t JobPool::cancelledCount() const
  {
    return m_cancelledCount.load();
  }

  bool JobPool::runsAfter(const Entry& a,
                          const Entry& b)
  {
    if (a.priority != b.priority)
    {
      return a.priority < b.priority;
    }

    return a.sequence > b.sequence;
  }

  void JobPool::enqueue(Entry entry)
  {
    if (!entry.job)
    {
      return;
    }

    {
      const std::lock_guard lock(m_mutex);

      entry.sequence = m_nextSequence++;
      m_queue.push_back(std::move(entry));
      std::push_heap(m_queue.begin(), m_queue.end(), runsAfter);
    }

    m_wakeup.notify_one();
  }

  void JobPool::workerLoop()
  {
    while (true)
    {
      Entry entry;

      {
        std::unique_lock lock(m_mutex);

        m_wakeup.wait(lock, [this] { return m_stopRequested || !m_queue.empty(); });

        if (m_stopRequested)
        {
          return;
        }

        std::pop_heap(m_queue.begin(), m_queue.end(), runsAfter);
        entry = std::move(m_queue.back());
        m_queue.pop_back();
      }

      if (entry.token && entry.token->isCancelled())
      {
        ++m_cancelledCount;
        continue;
      }

      try
      {
        entry.job();
      }
      catch (const std::exception& e)
      {
        ++m_failedCount;
        std::cerr << "Job failed: " << e.what() << '\n';
      }
      catch (...)
      {
        ++m_failedCount;
        std::cerr << "Job failed with an unknown exception\n";
      }
    }
  }

} // namespace vke
