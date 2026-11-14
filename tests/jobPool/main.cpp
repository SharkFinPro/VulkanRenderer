// Headless checks of the job pool, cancellation tokens and completion queue. No engine, no GPU: ctest runs this.
// Every wait has a timeout that records a failure instead of hanging.
#include "source/utilities/CompletionQueue.h"
#include "source/utilities/JobPool.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

  using vke::CancellationToken;
  using vke::CompletionQueue;
  using vke::Job;
  using vke::JobPool;

  constexpr auto TIMEOUT = std::chrono::seconds(10);

  // Timed waits use the system clock: GCC 11's ThreadSanitizer does not see steady-clock waits release their mutex
  // and reports false races.
  std::chrono::system_clock::time_point timeoutDeadline()
  {
    return std::chrono::system_clock::now() + TIMEOUT;
  }

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const char* name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  // Blocks jobs until released.
  class Gate {
  public:
    void release()
    {
      {
        const std::lock_guard lock(m_mutex);
        m_open = true;
      }

      m_cv.notify_all();
    }

    bool wait()
    {
      std::unique_lock lock(m_mutex);
      return m_cv.wait_until(lock, timeoutDeadline(), [this] { return m_open; });
    }

  private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_open = false;
  };

  // Counts events and lets the main thread wait for a number of them.
  class Counter {
  public:
    void add()
    {
      {
        const std::lock_guard lock(m_mutex);
        ++m_count;
      }

      m_cv.notify_all();
    }

    bool waitFor(const int target)
    {
      std::unique_lock lock(m_mutex);
      return m_cv.wait_until(lock, timeoutDeadline(), [this, target] { return m_count >= target; });
    }

  private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count = 0;
  };

  // Occupies a pool's worker until the gate opens; returns once the worker is inside the job.
  void blockWorker(JobPool& pool,
                   Gate& gate,
                   Counter& started)
  {
    pool.submit([&gate, &started] {
      started.add();
      gate.wait();
    });
  }

  void checkEveryJobRunsOnce()
  {
    constexpr int N = 10000;
    std::vector<std::atomic<int>> counts(N);
    Counter done;

    {
      JobPool pool(4);

      for (int i = 0; i < N; ++i)
      {
        pool.submit([&counts, &done, i] {
          ++counts[i];
          done.add();
        });
      }

      check(done.waitFor(N), "all jobs finish");
      check(pool.pendingCount() == 0, "nothing is pending once all jobs finished");
    }

    int total = 0;
    bool allOnce = true;
    for (const auto& count : counts)
    {
      total += count.load();
      allOnce = allOnce && count.load() == 1;
    }

    check(allOnce, "every job ran exactly once");
    check(total == N, "the run count equals the submit count");
  }

  void checkCompletionQueue()
  {
    constexpr int N = 5000;
    CompletionQueue<int> queue;
    std::vector<int> seen(N, 0);
    int received = 0;

    {
      JobPool pool(4);

      for (int i = 0; i < N; ++i)
      {
        pool.submit([&queue, i] { queue.push(i); });
      }

      const auto deadline = std::chrono::steady_clock::now() + TIMEOUT;
      while (received < N && std::chrono::steady_clock::now() < deadline)
      {
        queue.drain([&seen, &received](const int index) {
          ++seen[index];
          ++received;
        });
        std::this_thread::yield();
      }
    }

    check(received == N, "every result reaches the completion queue");
    check(std::all_of(seen.begin(), seen.end(), [](const int count) { return count == 1; }),
          "every result is delivered exactly once");
    check(queue.size() == 0, "nothing is left in the completion queue");

  }

  void checkCompletionQueueDrain()
  {
    CompletionQueue<int> ordered;
    for (int i = 0; i < 10; ++i)
    {
      ordered.push(i);
    }

    std::vector<int> order;
    ordered.drain([&order](const int value) { order.push_back(value); });
    std::vector<int> expected(10);
    std::iota(expected.begin(), expected.end(), 0);
    check(order == expected, "drain delivers in push order");

    // A callback that pushes would deadlock if drain held the lock; the push must land in the next drain.
    CompletionQueue<int> reentrant;
    reentrant.push(1);
    std::vector<int> first;
    reentrant.drain([&reentrant, &first](const int value) {
      first.push_back(value);
      reentrant.push(value + 1);
    });
    check(first == std::vector<int>{ 1 }, "a push made while draining is not delivered by the same drain");
    check(reentrant.size() == 1, "a push made while draining waits for the next drain");

    std::vector<int> second;
    reentrant.drain([&second](const int value) { second.push_back(value); });
    check(second == std::vector<int>{ 2 }, "a push made while draining arrives on the next drain");

    CompletionQueue<std::unique_ptr<int>> moveOnly;
    moveOnly.push(std::make_unique<int>(5));
    int got = 0;
    moveOnly.drain([&got](std::unique_ptr<int>& value) { got = *value; });
    check(got == 5, "the completion queue holds move-only results");
  }

  void checkPriorityOrder()
  {
    Gate gate;
    Counter started;
    Counter done;
    std::mutex orderMutex;
    std::vector<int> order;
    JobPool pool(1);

    blockWorker(pool, gate, started);
    check(started.waitFor(1), "the gate job starts");

    const auto record = [&](const int id) {
      return [&, id] {
        {
          const std::lock_guard lock(orderMutex);
          order.push_back(id);
        }

        done.add();
      };
    };

    const int priorities[] = { 0, 5, 0, 10, 5, 10, -1 };
    for (int id = 0; id < 7; ++id)
    {
      pool.submit(record(id), priorities[id]);
    }
    // No priority given means 0, after the earlier priority 0 jobs
    pool.submit(record(7));

    check(pool.pendingCount() == 8, "queued jobs are pending while the worker is blocked");

    gate.release();
    check(done.waitFor(8), "prioritized jobs finish");

    const std::vector<int> expected = { 3, 5, 1, 4, 0, 2, 7, 6 };
    check(order == expected, "higher priority runs first, FIFO within a priority");
  }

  // Many jobs, so the heap has to keep ties in submission order
  void checkFifoAcrossManyJobs()
  {
    constexpr int N = 600;
    Gate gate;
    Counter started;
    Counter done;
    std::mutex orderMutex;
    std::vector<int> order;
    JobPool pool(1);

    blockWorker(pool, gate, started);
    check(started.waitFor(1), "the second gate job starts");

    std::vector<int> expected(N);
    std::iota(expected.begin(), expected.end(), 0);

    for (int id = 0; id < N; ++id)
    {
      pool.submit([&, id] {
        {
          const std::lock_guard lock(orderMutex);
          order.push_back(id);
        }

        done.add();
      }, id % 3);
    }

    std::stable_sort(expected.begin(), expected.end(), [](const int a, const int b) { return a % 3 > b % 3; });

    gate.release();
    check(done.waitFor(N), "the many prioritized jobs finish");
    check(order == expected, "ties keep submission order across many jobs");
  }

  void checkCancellation()
  {
    // Declared before the pool, so they outlive its workers
    Gate gate;
    Counter started;
    Counter done;
    std::atomic<int> cancelledRan{ 0 };
    std::atomic<int> keptRan{ 0 };
    JobPool pool(1);

    blockWorker(pool, gate, started);
    check(started.waitFor(1), "the gate job starts before cancellation");

    CancellationToken token;
    check(!token.isCancelled(), "a new token is not cancelled");

    for (int i = 0; i < 5; ++i)
    {
      pool.submit([&cancelledRan] { ++cancelledRan; }, token);
    }

    const CancellationToken liveToken;
    pool.submit([&keptRan, &done] {
      ++keptRan;
      done.add();
    }, liveToken);

    pool.submit([&keptRan, &done] {
      ++keptRan;
      done.add();
    });

    // A copy shares the flag, and cancelling twice is harmless
    const CancellationToken copy = token;
    copy.cancel();
    token.cancel();
    check(token.isCancelled() && copy.isCancelled(), "cancelling a copy cancels every copy");

    gate.release();
    check(done.waitFor(2), "jobs queued after the cancelled ones run");

    check(cancelledRan == 0, "cancelled jobs never ran");
    check(pool.cancelledCount() == 5, "cancelledCount counts the dropped jobs");
    check(keptRan == 2, "uncancelled jobs ran");
    check(pool.failedCount() == 0, "a cancelled job is not a failure");
    check(!liveToken.isCancelled(), "cancelling one token leaves others alone");
  }

  void checkDestructionWithFullQueue()
  {
    constexpr int WORKERS = 2;
    constexpr int QUEUED = 1000;
    const auto sentinel = std::make_shared<int>(0);
    std::atomic<int> ran{ 0 };
    Gate gate;
    std::thread releaser;

    Counter started;

    {
      JobPool pool(WORKERS);

      for (int i = 0; i < WORKERS; ++i)
      {
        blockWorker(pool, gate, started);
      }

      check(started.waitFor(WORKERS), "both workers are blocked by gate jobs");

      for (int i = 0; i < QUEUED; ++i)
      {
        pool.submit([sentinel, &ran] { ++ran; });
      }

      check(pool.pendingCount() == QUEUED, "the queue is full when the pool is destroyed");

      // Opens the gate well after the destructor below has requested stop
      releaser = std::thread([&gate] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        gate.release();
      });
    }

    releaser.join();

    check(sentinel.use_count() == 1, "queued jobs are destroyed with the pool");
    check(ran < QUEUED, "queued jobs are not run by the destructor");
  }

  void checkThrowingJobs()
  {
    Counter done;
    std::atomic<bool> ran{ false };
    JobPool pool(1);

    pool.submit([] { throw std::runtime_error("expected failure from a test job"); });
    pool.submit([] { throw 42; });
    pool.submit([&ran, &done] {
      ran = true;
      done.add();
    });

    check(done.waitFor(1), "a job after throwing jobs still runs");
    check(ran, "the worker survived the throwing jobs");
    check(pool.failedCount() == 2, "both throwing jobs are counted as failures");
  }

  void checkWorkerCounts()
  {
    check(JobPool::defaultWorkerCount() >= 1, "the default worker count is at least one");

    {
      const JobPool pool(3);
      check(pool.workerCount() == 3, "an explicit worker count is honored");
    }

    {
      const JobPool pool(0);
      check(pool.workerCount() == JobPool::defaultWorkerCount(), "zero means the default worker count");
    }
  }

  void checkJobIsMoveOnly()
  {
    static_assert(!std::is_copy_constructible_v<Job>);
    static_assert(std::is_nothrow_move_constructible_v<Job>);

    int result = 0;
    Job job([&result, value = std::make_unique<int>(7)] { result = *value; });
    check(static_cast<bool>(job), "a job holding a move-only callable is valid");

    Job moved = std::move(job);
    check(!job && static_cast<bool>(moved), "moving a job empties the source");

    moved();
    check(result == 7, "a moved job calls the callable");

    check(!Job(), "a default job is empty");

    Counter done;
    std::atomic<int> poolResult{ 0 };
    {
      JobPool pool(1);
      pool.submit([&poolResult, &done, value = std::make_unique<int>(9)] {
        poolResult = *value;
        done.add();
      });
      check(done.waitFor(1), "a move-only job can be submitted");
    }
    check(poolResult == 9, "a submitted move-only job ran");
  }

  void checkEmptyJobIsIgnored()
  {
    Counter done;
    JobPool pool(1);

    pool.submit(Job{});
    pool.submit(Job{}, CancellationToken{}, 3);
    check(pool.pendingCount() == 0, "an empty job is not queued");

    pool.submit([&done] { done.add(); });
    check(done.waitFor(1), "the worker survives an empty job submit");
    check(pool.failedCount() == 0, "an empty job is not a failure");
  }

  // Drains, throwing at the result 2; returns whether the exception reached the caller.
  bool drainThrowingAtTwo(CompletionQueue<int>& queue,
                          std::vector<int>& delivered)
  {
    try
    {
      queue.drain([&delivered](const int value) {
        delivered.push_back(value);
        if (value == 2)
        {
          throw std::runtime_error("drain callback failed");
        }
      });
    }
    catch (const std::runtime_error&)
    {
      return true;
    }

    return false;
  }

  void checkThrowingDrain()
  {
    CompletionQueue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    std::vector<int> delivered;
    check(drainThrowingAtTwo(queue, delivered), "an exception from the drain callback propagates");
    check(delivered == std::vector<int>{ 1, 2 }, "a throwing drain stops at the failing result");

    queue.push(4);
    std::vector<int> next;
    queue.drain([&next](const int value) { next.push_back(value); });
    check(next == std::vector<int>{ 4 }, "results from a throwing drain are not delivered again");
  }

} // namespace

int main()
{
  checkEveryJobRunsOnce();
  checkCompletionQueue();
  checkCompletionQueueDrain();
  checkPriorityOrder();
  checkFifoAcrossManyJobs();
  checkCancellation();
  checkDestructionWithFullQueue();
  checkThrowingJobs();
  checkWorkerCounts();
  checkJobIsMoveOnly();
  checkEmptyJobIsIgnored();
  checkThrowingDrain();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
