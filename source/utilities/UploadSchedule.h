#ifndef VKE_UPLOADSCHEDULE_H
#define VKE_UPLOADSCHEDULE_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <utility>

namespace vke {

  // A FIFO of pending uploads with a per-frame byte budget. Engine-independent so the scheduling rules are unit tested
  // without a GPU.
  template <typename T>
  class UploadSchedule {
  public:
    // Appends at the back; the oldest item is taken first.
    void push(T item,
              const uint64_t bytes)
    {
      m_items.push_back(Entry { std::move(item), bytes });
      m_pendingBytes += bytes;
    }

    // Pops, oldest first, items while their running total stays within budgetBytes, calling onTake(T&&) for each, and
    // returns the bytes taken. It stops at the first item that does not fit and never skips ahead, so order is kept.
    // An oversized head is taken alone, and only when nothing was taken yet in this call, so it never waits forever.
    // A budget of 0 therefore takes one non-empty item per call.
    template <typename F>
    uint64_t take(const uint64_t budgetBytes,
                  F&& onTake)
    {
      uint64_t taken = 0;
      bool any = false;

      while (!m_items.empty())
      {
        const uint64_t bytes = m_items.front().bytes;
        const bool fits = taken <= budgetBytes && bytes <= budgetBytes - taken;

        if (!fits && any)
        {
          break;
        }

        T item = std::move(m_items.front().item);
        m_items.pop_front();

        any = true;
        taken += bytes;
        m_pendingBytes -= bytes;

        onTake(std::move(item));

        if (!fits)
        {
          break;
        }
      }

      return taken;
    }

    [[nodiscard]] bool empty() const
    {
      return m_items.empty();
    }

    [[nodiscard]] size_t size() const
    {
      return m_items.size();
    }

    [[nodiscard]] uint64_t pendingBytes() const
    {
      return m_pendingBytes;
    }

    // Drops everything without taking it.
    void clear()
    {
      m_items.clear();
      m_pendingBytes = 0;
    }

  private:
    struct Entry {
      T item;
      uint64_t bytes;
    };

    std::deque<Entry> m_items;

    uint64_t m_pendingBytes = 0;
  };

} // namespace vke

#endif //VKE_UPLOADSCHEDULE_H
