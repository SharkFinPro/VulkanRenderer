#ifndef VKE_FRAMESCHEDULER_H
#define VKE_FRAMESCHEDULER_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  // Owns the frame synchronization objects and the per-frame submit/present policy.
  //
  // GPU↔GPU ordering within a frame (compute → offscreen → swapchain) and CPU frame pacing are
  // both driven by a single timeline semaphore: each frame signals three monotonically
  // increasing values, one per submission. Binary semaphores remain only where WSI requires
  // them (image acquisition and presentation).
  class FrameScheduler {
  public:
    explicit FrameScheduler(std::shared_ptr<LogicalDevice> logicalDevice);

    // Advances to the next frame, blocking until the GPU has fully finished the frame that
    // previously used this frame-in-flight slot. Must be called once per frame, before any
    // recording or submission for that frame.
    void beginFrame();

    // Frame-in-flight index of the frame started by the latest beginFrame().
    [[nodiscard]] uint32_t getCurrentFrame() const;

    void submitComputeCommandBuffer(vk::CommandBuffer commandBuffer) const;

    void submitOffscreenCommandBuffer(vk::CommandBuffer commandBuffer) const;

    void submitSwapchainCommandBuffer(uint32_t imageIndex,
                                      vk::CommandBuffer commandBuffer) const;

    // Blocks until this frame's offscreen submission has finished executing
    // (used for the mouse picking readback).
    void waitForOffscreenWork() const;

    vk::Result acquireNextImage(vk::SwapchainKHR swapchain,
                                uint32_t* imageIndex) const;

    vk::Result queuePresent(vk::SwapchainKHR swapchain,
                            uint32_t imageIndex) const;

    // Render-finished semaphores are indexed by swapchain image (not frame in flight), because
    // presentation may still be waiting on them after the frame's other work has completed.
    // Must be called whenever a swapchain is (re)created. Requires an idle device.
    void updateRenderFinishedSemaphores(uint32_t swapchainImageCount);

    // Brings the timeline up to the current frame's final value after the frame was abandoned
    // (out-of-date swapchain at acquire), so later frame-pacing waits cannot deadlock.
    // Requires an idle device (no in-flight timeline signals).
    void completeAbortedFrame() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::Semaphore m_timelineSemaphore = nullptr;

    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores; // indexed by frame in flight
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores; // indexed by swapchain image

    // Monotonic frame counter; the first beginFrame() makes it 1.
    uint64_t m_frameNumber = 0;

    // Timeline values signaled per frame, offset from frameBaseValue().
    static constexpr uint64_t s_signalsPerFrame = 3;
    static constexpr uint64_t s_computeFinished = 1;
    static constexpr uint64_t s_offscreenFinished = 2;
    static constexpr uint64_t s_swapchainFinished = 3;

    [[nodiscard]] uint64_t frameBaseValue() const;

    void waitForTimelineValue(uint64_t value) const;

    void createSemaphores();
  };

} // namespace vke

#endif //VKE_FRAMESCHEDULER_H
