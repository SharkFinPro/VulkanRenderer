#ifndef VKE_DEFERREDDESTRUCTIONQUEUE_H
#define VKE_DEFERREDDESTRUCTIONQUEUE_H

#include <cstdint>
#include <memory>
#include <vector>

namespace vke {

  // Holds resources the GPU may still be using and destroys each one once the last frame that could
  // reference it has completed, so destroying a resource never needs a device-wide wait.
  class DeferredDestructionQueue {
  public:
    // Stamps the resource with the current frame. A resource pushed at any point after beginFrame(F)
    // may be used by frame F, so it is destroyed once F has completed. Null is ignored.
    void push(std::shared_ptr<void> resource);

    // Records frameNumber as the current frame, then destroys every entry stamped lastCompletedFrame or
    // earlier.
    void beginFrame(uint64_t frameNumber,
                    uint64_t lastCompletedFrame);

    // Destroys everything. The caller guarantees the device is idle.
    void destroyAll();

    [[nodiscard]] bool empty() const;

  private:
    struct Entry {
      uint64_t frameNumber;
      std::shared_ptr<void> resource;
    };

    std::vector<Entry> m_entries;

    uint64_t m_currentFrame = 0;
  };

} // namespace vke

#endif //VKE_DEFERREDDESTRUCTIONQUEUE_H
