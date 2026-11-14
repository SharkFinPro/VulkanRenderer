#ifndef VKE_UPLOADQUEUE_H
#define VKE_UPLOADQUEUE_H

#include "../../utilities/UploadSchedule.h"
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  // Shared by an upload's owner (a Mesh) and the queue, which holds it weakly: an owner that is gone by the time its
  // request would be recorded has its request dropped, so the queue never writes into destroyed buffers.
  struct UploadState {
    bool ready = false;
  };

  struct UploadCopy {
    vk::Buffer destination;
    vk::DeviceSize sourceOffset = 0;
    vk::DeviceSize destinationOffset = 0;
    vk::DeviceSize size = 0;
  };

  // One bottom-level build, recorded after the copies of its request. The scratch buffer is allocated at enqueue time.
  struct UploadBlasBuild {
    vk::DeviceAddress vertexAddress = 0;
    vk::DeviceSize vertexStride = 0;
    uint32_t maxVertex = 0;
    vk::DeviceAddress indexAddress = 0;
    uint32_t primitiveCount = 0;
    vk::AccelerationStructureKHR destination;
    vk::raii::DeviceMemory scratchMemory = nullptr;
    vk::raii::Buffer scratchBuffer = nullptr;
    vk::DeviceAddress scratchAddress = 0;
  };

  struct UploadRequest {
    vk::raii::DeviceMemory stagingMemory = nullptr;
    vk::raii::Buffer stagingBuffer = nullptr;
    // The staging memory stays mapped for the life of the request; fill it with write().
    void* mapped = nullptr;
    vk::DeviceSize stagingSize = 0;
    std::vector<UploadCopy> copies;
    std::optional<UploadBlasBuild> blas;
    std::weak_ptr<UploadState> owner;

    void write(vk::DeviceSize offset,
               const void* data,
               vk::DeviceSize size) const;
  };

  // Non-blocking uploads into device-local buffers. A request carries CPU data in a staging buffer and the copies (and
  // optional BLAS build) that move it; record() puts a frame's share of them at the start of the offscreen command
  // buffer, so creating a resource never waits on a queue. Frame thread only.
  class UploadQueue {
  public:
    UploadQueue(std::shared_ptr<LogicalDevice> logicalDevice,
                vk::DeviceSize bytesPerFrame);

    // Allocates the host-visible staging buffer for a request owned by owner. Fill it with UploadRequest::write, add
    // the copies, then enqueue().
    [[nodiscard]] UploadRequest createRequest(std::weak_ptr<UploadState> owner,
                                              vk::DeviceSize stagingSize) const;

    void enqueue(UploadRequest request);

    // Marks the owners of the requests recorded in the previous frame ready: those copies are ahead of every command
    // recorded from now on, on the same queue, behind a barrier. Called right after the frame scheduler's beginFrame.
    void beginFrame();

    // Records, oldest first, as many pending requests as the per-frame budget allows, then one barrier making the
    // copied data visible to vertex input, shaders and BLAS builds, then the BLAS builds and a barrier after them.
    // Records nothing when nothing is pending. A frame that skips the offscreen pass records nothing either, so
    // uploads wait for the next one. Each recorded request's staging and scratch buffers are retired through the
    // deferred destruction queue.
    void record(const CommandBuffer& commandBuffer);

    // Drops pending requests without recording them. For teardown with an idle device.
    void clear();

    void setBytesPerFrame(vk::DeviceSize bytesPerFrame);

    [[nodiscard]] vk::DeviceSize getBytesPerFrame() const;

    [[nodiscard]] size_t getPendingCount() const;

    [[nodiscard]] vk::DeviceSize getPendingBytes() const;

  private:
    // Destroyed in reverse order: scratch before staging, each buffer before its memory.
    struct RetiredRequest {
      vk::raii::DeviceMemory stagingMemory = nullptr;
      vk::raii::Buffer stagingBuffer = nullptr;
      vk::raii::DeviceMemory scratchMemory = nullptr;
      vk::raii::Buffer scratchBuffer = nullptr;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::DeviceSize m_bytesPerFrame;

    UploadSchedule<UploadRequest> m_schedule;

    // Scratch reused every frame.
    std::vector<UploadRequest> m_recording;
    std::vector<std::weak_ptr<UploadState>> m_recordedThisFrame;
    std::vector<std::weak_ptr<UploadState>> m_recordedLastFrame;

    void recordCopyBarrier(const CommandBuffer& commandBuffer) const;

    void recordBlasBuilds(const CommandBuffer& commandBuffer) const;

    void retireRecorded();
  };

} // namespace vke

#endif //VKE_UPLOADQUEUE_H
