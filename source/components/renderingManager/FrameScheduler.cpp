#include "FrameScheduler.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {

  FrameScheduler::FrameScheduler(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createSemaphores();
  }

  void FrameScheduler::beginFrame()
  {
    m_frameNumber++;

    const uint64_t maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

    if (m_frameNumber <= maxFramesInFlight)
    {
      return;
    }

    // Wait until the frame that last used this frame-in-flight slot has fully completed, so
    // its command buffers, uniform buffers and per-frame images can be reused.
    const uint64_t lastFrameInSlot = m_frameNumber - maxFramesInFlight;
    waitForTimelineValue((lastFrameInSlot - 1) * s_signalsPerFrame + s_swapchainFinished);
  }

  uint32_t FrameScheduler::getCurrentFrame() const
  {
    return static_cast<uint32_t>((m_frameNumber - 1) % m_logicalDevice->getMaxFramesInFlight());
  }

  void FrameScheduler::submitComputeCommandBuffer(const vk::CommandBuffer commandBuffer) const
  {
    const vk::CommandBufferSubmitInfo commandBufferSubmitInfo {
      .commandBuffer = commandBuffer
    };

    const vk::SemaphoreSubmitInfo signalSemaphoreInfo {
      .semaphore = *m_timelineSemaphore,
      .value = frameBaseValue() + s_computeFinished,
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    };

    const vk::SubmitInfo2 submitInfo {
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &commandBufferSubmitInfo,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos = &signalSemaphoreInfo
    };

    m_logicalDevice->submitToComputeQueue(submitInfo);
  }

  void FrameScheduler::submitOffscreenCommandBuffer(const vk::CommandBuffer commandBuffer) const
  {
    // Compute writes particle vertex buffers consumed by the offscreen pass.
    const vk::SemaphoreSubmitInfo waitSemaphoreInfo {
      .semaphore = *m_timelineSemaphore,
      .value = frameBaseValue() + s_computeFinished,
      .stageMask = vk::PipelineStageFlagBits2::eVertexInput
    };

    const vk::CommandBufferSubmitInfo commandBufferSubmitInfo {
      .commandBuffer = commandBuffer
    };

    const vk::SemaphoreSubmitInfo signalSemaphoreInfo {
      .semaphore = *m_timelineSemaphore,
      .value = frameBaseValue() + s_offscreenFinished,
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    };

    const vk::SubmitInfo2 submitInfo {
      .waitSemaphoreInfoCount = 1,
      .pWaitSemaphoreInfos = &waitSemaphoreInfo,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &commandBufferSubmitInfo,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos = &signalSemaphoreInfo
    };

    m_logicalDevice->submitToGraphicsQueue(submitInfo);
  }

  void FrameScheduler::submitSwapchainCommandBuffer(const uint32_t imageIndex,
                                                    const vk::CommandBuffer commandBuffer) const
  {
    // The offscreen resolve image is sampled in the fragment shader (scene view /
    // offscreenToSwapchain); the swapchain image is first written as a color attachment.
    const std::array waitSemaphoreInfos {
      vk::SemaphoreSubmitInfo {
        .semaphore = *m_timelineSemaphore,
        .value = frameBaseValue() + s_offscreenFinished,
        .stageMask = vk::PipelineStageFlagBits2::eFragmentShader
      },
      vk::SemaphoreSubmitInfo {
        .semaphore = *m_imageAvailableSemaphores[getCurrentFrame()],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
      }
    };

    const vk::CommandBufferSubmitInfo commandBufferSubmitInfo {
      .commandBuffer = commandBuffer
    };

    const std::array signalSemaphoreInfos {
      vk::SemaphoreSubmitInfo {
        .semaphore = *m_timelineSemaphore,
        .value = frameBaseValue() + s_swapchainFinished,
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands
      },
      vk::SemaphoreSubmitInfo {
        .semaphore = *m_renderFinishedSemaphores[imageIndex],
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands
      }
    };

    const vk::SubmitInfo2 submitInfo {
      .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreInfos.size()),
      .pWaitSemaphoreInfos = waitSemaphoreInfos.data(),
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &commandBufferSubmitInfo,
      .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size()),
      .pSignalSemaphoreInfos = signalSemaphoreInfos.data()
    };

    m_logicalDevice->submitToGraphicsQueue(submitInfo);
  }

  void FrameScheduler::waitForOffscreenWork() const
  {
    waitForTimelineValue(frameBaseValue() + s_offscreenFinished);
  }

  vk::Result FrameScheduler::acquireNextImage(const vk::SwapchainKHR swapchain,
                                              uint32_t* imageIndex) const
  {
    const vk::AcquireNextImageInfoKHR acquireInfo {
      .swapchain = swapchain,
      .timeout = UINT64_MAX,
      .semaphore = *m_imageAvailableSemaphores[getCurrentFrame()],
      .fence = nullptr,
      .deviceMask = 1
    };

    auto [result, index] = m_logicalDevice->acquireNextImage(acquireInfo);
    *imageIndex = index;

    return result;
  }

  vk::Result FrameScheduler::queuePresent(const vk::SwapchainKHR swapchain,
                                          const uint32_t imageIndex) const
  {
    const vk::PresentInfoKHR presentInfo {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex],
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
      .pResults = nullptr
    };

    return m_logicalDevice->queuePresent(presentInfo);
  }

  void FrameScheduler::updateRenderFinishedSemaphores(const uint32_t swapchainImageCount)
  {
    m_renderFinishedSemaphores.clear();
    m_renderFinishedSemaphores.reserve(swapchainImageCount);

    constexpr vk::SemaphoreCreateInfo semaphoreInfo {};

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
      m_renderFinishedSemaphores.emplace_back(m_logicalDevice->createSemaphore(semaphoreInfo));
    }
  }

  void FrameScheduler::completeAbortedFrame() const
  {
    const uint64_t frameEndValue = m_frameNumber * s_signalsPerFrame;

    if (m_timelineSemaphore.getCounterValue() >= frameEndValue)
    {
      return;
    }

    const vk::SemaphoreSignalInfo signalInfo {
      .semaphore = *m_timelineSemaphore,
      .value = frameEndValue
    };

    m_logicalDevice->signalSemaphore(signalInfo);
  }

  uint64_t FrameScheduler::frameBaseValue() const
  {
    return (m_frameNumber - 1) * s_signalsPerFrame;
  }

  void FrameScheduler::waitForTimelineValue(const uint64_t value) const
  {
    const vk::SemaphoreWaitInfo waitInfo {
      .semaphoreCount = 1,
      .pSemaphores = &*m_timelineSemaphore,
      .pValues = &value
    };

    m_logicalDevice->waitSemaphores(waitInfo);
  }

  void FrameScheduler::createSemaphores()
  {
    constexpr vk::SemaphoreTypeCreateInfo timelineTypeCreateInfo {
      .semaphoreType = vk::SemaphoreType::eTimeline,
      .initialValue = 0
    };

    const vk::SemaphoreCreateInfo timelineCreateInfo {
      .pNext = &timelineTypeCreateInfo
    };

    m_timelineSemaphore = m_logicalDevice->createSemaphore(timelineCreateInfo);

    const auto maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

    m_imageAvailableSemaphores.reserve(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
      constexpr vk::SemaphoreCreateInfo semaphoreInfo {};

      m_imageAvailableSemaphores.emplace_back(m_logicalDevice->createSemaphore(semaphoreInfo));
    }
  }

} // namespace vke
