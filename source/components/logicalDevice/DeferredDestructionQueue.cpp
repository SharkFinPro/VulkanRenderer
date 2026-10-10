#include "DeferredDestructionQueue.h"
#include <algorithm>
#include <iterator>
#include <utility>

namespace vke {

  void DeferredDestructionQueue::push(std::shared_ptr<void> resource)
  {
    if (!resource)
    {
      return;
    }

    m_entries.push_back({ m_currentFrame, std::move(resource) });
  }

  void DeferredDestructionQueue::beginFrame(const uint64_t frameNumber,
                                            const uint64_t lastCompletedFrame)
  {
    m_currentFrame = frameNumber;

    // A destructor can retire its own members, which appends to m_entries, so the expired entries are moved
    // out before any of them is destroyed.
    std::vector<Entry> expired;

    const auto firstExpired = std::stable_partition(m_entries.begin(), m_entries.end(), [lastCompletedFrame](const Entry& entry) {
      return entry.frameNumber > lastCompletedFrame;
    });

    expired.assign(std::make_move_iterator(firstExpired), std::make_move_iterator(m_entries.end()));
    m_entries.erase(firstExpired, m_entries.end());
  }

  void DeferredDestructionQueue::destroyAll()
  {
    while (!m_entries.empty())
    {
      std::vector<Entry> entries;
      entries.swap(m_entries);
    }
  }

  bool DeferredDestructionQueue::empty() const
  {
    return m_entries.empty();
  }

} // namespace vke
