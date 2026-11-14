#include "UploadQueue.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../../utilities/Buffers.h"
#include <cstring>
#include <utility>

namespace vke {

  void UploadRequest::write(const vk::DeviceSize offset,
                            const void* data,
                            const vk::DeviceSize size) const
  {
    std::memcpy(static_cast<std::byte*>(mapped) + offset, data, size);
  }

  UploadQueue::UploadQueue(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::DeviceSize bytesPerFrame)
    : m_logicalDevice(std::move(logicalDevice)),
      m_bytesPerFrame(bytesPerFrame)
  {
  }

  UploadRequest UploadQueue::createRequest(std::weak_ptr<UploadState> owner,
                                           const vk::DeviceSize stagingSize) const
  {
    UploadRequest request;
    request.stagingSize = stagingSize;
    request.owner = std::move(owner);

    Buffers::createBuffer(
      m_logicalDevice,
      stagingSize,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      request.stagingBuffer,
      request.stagingMemory
    );

    request.mapped = request.stagingMemory.mapMemory(0, vk::WholeSize);

    return request;
  }

  void UploadQueue::enqueue(UploadRequest request)
  {
    const vk::DeviceSize bytes = request.stagingSize;

    m_schedule.push(std::move(request), bytes);
  }

  void UploadQueue::beginFrame()
  {
    // Swapped rather than copied so neither vector reallocates in steady state.
    m_recordedLastFrame.swap(m_recordedThisFrame);
    m_recordedThisFrame.clear();

    for (const auto& weakState : m_recordedLastFrame)
    {
      if (const auto state = weakState.lock())
      {
        state->ready = true;
      }
    }

    m_recordedLastFrame.clear();
  }

  void UploadQueue::record(const CommandBuffer& commandBuffer)
  {
    if (m_schedule.empty())
    {
      return;
    }

    m_schedule.take(m_bytesPerFrame, [this](UploadRequest&& request) {
      m_recording.push_back(std::move(request));
    });

    bool copied = false;
    bool anyBlas = false;

    for (UploadRequest& request : m_recording)
    {
      if (request.owner.expired())
      {
        // Never seen by the GPU, so it is destroyed with the request below instead of being retired.
        request.copies.clear();
        request.blas.reset();
        continue;
      }

      for (const UploadCopy& copy : request.copies)
      {
        const vk::BufferCopy region {
          .srcOffset = copy.sourceOffset,
          .dstOffset = copy.destinationOffset,
          .size = copy.size
        };

        commandBuffer.copyBuffer(*request.stagingBuffer, copy.destination, region);

        copied = true;
      }

      anyBlas = anyBlas || request.blas.has_value();

      m_recordedThisFrame.push_back(request.owner);
    }

    if (copied)
    {
      recordCopyBarrier(commandBuffer);
    }

    if (anyBlas)
    {
      recordBlasBuilds(commandBuffer);
    }

    retireRecorded();
  }

  void UploadQueue::clear()
  {
    m_schedule.clear();
    m_recording.clear();
    m_recordedThisFrame.clear();
    m_recordedLastFrame.clear();
  }

  void UploadQueue::setBytesPerFrame(const vk::DeviceSize bytesPerFrame)
  {
    m_bytesPerFrame = bytesPerFrame;
  }

  vk::DeviceSize UploadQueue::getBytesPerFrame() const
  {
    return m_bytesPerFrame;
  }

  size_t UploadQueue::getPendingCount() const
  {
    return m_schedule.size();
  }

  vk::DeviceSize UploadQueue::getPendingBytes() const
  {
    return m_schedule.pendingBytes();
  }

  void UploadQueue::recordCopyBarrier(const CommandBuffer& commandBuffer) const
  {
    // Every graphics stage, since a pipeline may read a mesh's buffers as storage in any of them (geometry shaders do)
    vk::PipelineStageFlags2 dstStages = vk::PipelineStageFlagBits2::eAllGraphics |
                                        vk::PipelineStageFlagBits2::eComputeShader;

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      dstStages |= vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR |
                   vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
    }

    const vk::MemoryBarrier2 barrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
      .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
      .dstStageMask = dstStages,
      .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead |
                       vk::AccessFlagBits2::eIndexRead |
                       vk::AccessFlagBits2::eShaderStorageRead |
                       vk::AccessFlagBits2::eShaderRead
    };

    const vk::DependencyInfo dependencyInfo {
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier(dependencyInfo);
  }

  void UploadQueue::recordBlasBuilds(const CommandBuffer& commandBuffer) const
  {
    for (const UploadRequest& request : m_recording)
    {
      if (!request.blas)
      {
        continue;
      }

      const UploadBlasBuild& build = *request.blas;

      vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData {};
      trianglesData.vertexFormat = vk::Format::eR32G32B32Sfloat;
      trianglesData.vertexData.deviceAddress = build.vertexAddress;
      trianglesData.vertexStride = build.vertexStride;
      trianglesData.maxVertex = build.maxVertex;
      trianglesData.indexType = vk::IndexType::eUint32;
      trianglesData.indexData.deviceAddress = build.indexAddress;

      vk::AccelerationStructureGeometryKHR geometry {};
      geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
      geometry.geometry.triangles = trianglesData;
      geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;

      vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {};
      buildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
      buildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
      buildGeometryInfo.geometryCount = 1;
      buildGeometryInfo.pGeometries = &geometry;
      buildGeometryInfo.dstAccelerationStructure = build.destination;
      buildGeometryInfo.scratchData.deviceAddress = build.scratchAddress;

      const vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
        .primitiveCount = build.primitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
      };

      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    }

    const vk::MemoryBarrier2 barrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
      .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
      .dstStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR |
                      vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
      .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR
    };

    const vk::DependencyInfo dependencyInfo {
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier(dependencyInfo);
  }

  void UploadQueue::retireRecorded()
  {
    for (UploadRequest& request : m_recording)
    {
      if (request.copies.empty() && !request.blas)
      {
        continue;
      }

      RetiredRequest retired;
      retired.stagingMemory = std::move(request.stagingMemory);
      retired.stagingBuffer = std::move(request.stagingBuffer);

      if (request.blas)
      {
        retired.scratchMemory = std::move(request.blas->scratchMemory);
        retired.scratchBuffer = std::move(request.blas->scratchBuffer);
      }

      // The GPU reads these during this frame's submission.
      m_logicalDevice->retireHandles(std::move(retired));
    }

    m_recording.clear();
  }

} // namespace vke
