#ifndef VKE_COMPLETIONQUEUE_H
#define VKE_COMPLETIONQUEUE_H

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace vke {

  // Hands results from any thread (jobs) to one consumer (the frame thread), which drains them at one known point per
  // frame. This is the only route from a job to the GPU.
  template <typename T>
  class CompletionQueue {
  public:
    CompletionQueue() = default;

    CompletionQueue(const CompletionQueue&) = delete;
    CompletionQueue& operator=(const CompletionQueue&) = delete;

    // Any thread.
    void push(T value)
    {
      const std::lock_guard lock(m_mutex);

      m_pending.emplace_back(std::move(value));
    }

    // Calls f(T&) for everything pushed so far, in push order. f runs without the lock, so it may push; that lands in
    // the next drain. Call from one thread only, and not from inside f. Both vectors keep their capacity, so a steady
    // state allocates nothing. If f throws, the rest of this drain's results are dropped and the exception propagates.
    template <typename F>
    void drain(F&& f)
    {
      {
        const std::lock_guard lock(m_mutex);

        m_draining.swap(m_pending);
      }

      // Cleared on every exit, so a throwing f can't leave stale results to be delivered again
      struct ClearOnExit {
        std::vector<T>& values;
        ~ClearOnExit() { values.clear(); }
      } clearOnExit{ m_draining };

      for (T& value : m_draining)
      {
        f(value);
      }
    }

    // Results waiting for the next drain.
    [[nodiscard]] size_t size() const
    {
      const std::lock_guard lock(m_mutex);

      return m_pending.size();
    }

  private:
    mutable std::mutex m_mutex;
    std::vector<T> m_pending;
    std::vector<T> m_draining;
  };

} // namespace vke

#endif //VKE_COMPLETIONQUEUE_H
