#include "CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include <stdexcept>

namespace vke {
  CommandBuffer::CommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {}

  CommandBuffer::CommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                               vk::CommandPool& commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    CommandBuffer::allocateCommandBuffers(commandPool);
  }

  void CommandBuffer::setCurrentFrame(const uint32_t currentFrame)
  {
    m_currentFrame = currentFrame;
  }

  void CommandBuffer::record(const std::function<void()>& renderFunction) const
  {
    constexpr vk::CommandBufferBeginInfo beginInfo {};

    m_commandBuffers[m_currentFrame].begin(beginInfo);

    renderFunction();

    m_commandBuffers[m_currentFrame].end();
  }

  void CommandBuffer::resetCommandBuffer() const
  {
    m_commandBuffers[m_currentFrame].reset();
  }

  vk::CommandBuffer CommandBuffer::getCommandBuffer()
  {
    return m_commandBuffers[m_currentFrame];
  }

  void CommandBuffer::setViewport(const vk::Viewport& viewport) const
  {
    m_commandBuffers[m_currentFrame].setViewport(0, { viewport });
  }

  void CommandBuffer::setScissor(const vk::Rect2D& scissor) const
  {
    m_commandBuffers[m_currentFrame].setScissor(0, { scissor });
  }

  void CommandBuffer::beginRenderPass(const vk::RenderPassBeginInfo& renderPassBeginInfo) const
  {
    m_commandBuffers[m_currentFrame].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  }

  void CommandBuffer::endRenderPass() const
  {
    m_commandBuffers[m_currentFrame].endRenderPass();
  }

  void CommandBuffer::beginRendering(const vk::RenderingInfo& renderingInfo) const
  {
    m_commandBuffers[m_currentFrame].beginRendering(renderingInfo);
  }

  void CommandBuffer::endRendering() const
  {
    m_commandBuffers[m_currentFrame].endRendering();
  }

  void CommandBuffer::bindPipeline(const vk::PipelineBindPoint pipelineBindPoint,
                                   const vk::Pipeline& pipeline) const
  {
    m_commandBuffers[m_currentFrame].bindPipeline(pipelineBindPoint, pipeline);
  }

  void CommandBuffer::bindDescriptorSets(const vk::PipelineBindPoint pipelineBindPoint,
                                         const vk::PipelineLayout& pipelineLayout,
                                         const uint32_t firstSet,
                                         const std::vector<vk::DescriptorSet>& descriptorSets) const
  {
    m_commandBuffers[m_currentFrame].bindDescriptorSets(
      pipelineBindPoint,
      pipelineLayout,
      firstSet,
      descriptorSets,
      {}
    );
  }

  void CommandBuffer::dispatch(const uint32_t groupCountX,
                               const uint32_t groupCountY,
                               const uint32_t groupCountZ) const
  {
    m_commandBuffers[m_currentFrame].dispatch(groupCountX, groupCountY, groupCountZ);
  }

  void CommandBuffer::bindVertexBuffers(const uint32_t firstBinding,
                                        const std::vector<vk::Buffer>& buffers,
                                        const std::vector<vk::DeviceSize>& offsets) const
  {
    m_commandBuffers[m_currentFrame].bindVertexBuffers(firstBinding, buffers, offsets);
  }

  void CommandBuffer::bindIndexBuffer(const vk::Buffer& buffer,
                                      const vk::DeviceSize offset,
                                      const vk::IndexType indexType) const
  {
    m_commandBuffers[m_currentFrame].bindIndexBuffer(buffer, offset, indexType);
  }

  void CommandBuffer::draw(const uint32_t vertexCount,
                           const uint32_t instanceCount,
                           const uint32_t firstVertex,
                           const uint32_t firstInstance) const
  {
    m_commandBuffers[m_currentFrame].draw(vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void CommandBuffer::drawIndexed(const uint32_t indexCount,
                                  const uint32_t instanceCount,
                                  const uint32_t firstIndex,
                                  const int32_t vertexOffset,
                                  const uint32_t firstInstance) const
  {
    m_commandBuffers[m_currentFrame].drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  }

  void CommandBuffer::pipelineBarrier(const vk::PipelineStageFlags srcStageMask,
                                      const vk::PipelineStageFlags dstStageMask,
                                      const vk::DependencyFlags dependencyFlags,
                                      const std::vector<vk::MemoryBarrier>& memoryBarriers,
                                      const std::vector<vk::BufferMemoryBarrier>& bufferMemoryBarriers,
                                      const std::vector<vk::ImageMemoryBarrier>& imageMemoryBarriers) const
  {
    m_commandBuffers[m_currentFrame].pipelineBarrier(
      srcStageMask,
      dstStageMask,
      dependencyFlags,
      memoryBarriers,
      bufferMemoryBarriers,
      imageMemoryBarriers
    );
  }

  void CommandBuffer::clearAttachments(const std::vector<vk::ClearAttachment>& clearAttachments,
                                       const std::vector<vk::ClearRect>& clearRects) const
  {
    m_commandBuffers[m_currentFrame].clearAttachments(clearAttachments, clearRects);
  }

  void CommandBuffer::copyImageToBuffer(const vk::Image& srcImage,
                                        const vk::ImageLayout srcImageLayout,
                                        const vk::Buffer& dstBuffer,
                                        const std::vector<vk::BufferImageCopy>& regions) const
  {
    m_commandBuffers[m_currentFrame].copyImageToBuffer(srcImage, srcImageLayout, dstBuffer, regions);
  }

  void CommandBuffer::copyBufferToImage(const vk::Buffer& srcBuffer,
                                        const vk::Image& dstImage,
                                        const vk::ImageLayout dstImageLayout,
                                        const std::vector<vk::BufferImageCopy>& regions) const
  {
    m_commandBuffers[m_currentFrame].copyBufferToImage(srcBuffer, dstImage, dstImageLayout, regions);
  }

  void CommandBuffer::blitImage(const vk::Image& srcImage,
                                const vk::ImageLayout srcImageLayout,
                                const vk::Image& dstImage,
                                const vk::ImageLayout dstImageLayout,
                                const std::vector<vk::ImageBlit>& regions,
                                const vk::Filter filter) const
  {
    m_commandBuffers[m_currentFrame].blitImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regions, filter);
  }

  void CommandBuffer::copyBuffer(const vk::Buffer& srcBuffer,
                                 const vk::Buffer& dstBuffer,
                                 const std::vector<vk::BufferCopy>& regions) const
  {
    m_commandBuffers[m_currentFrame].copyBuffer(srcBuffer, dstBuffer, regions);
  }

  void CommandBuffer::buildAccelerationStructure(const vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                                                 const vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfo) const
  {
    m_commandBuffers[m_currentFrame].buildAccelerationStructuresKHR(
      buildGeometryInfo,
      buildRangeInfo
    );
  }

  void CommandBuffer::traceRays(const vk::StridedDeviceAddressRegionKHR& raygenShaderBindingTable,
                                const vk::StridedDeviceAddressRegionKHR& missShaderBindingTable,
                                const vk::StridedDeviceAddressRegionKHR& hitShaderBindingTable,
                                const vk::StridedDeviceAddressRegionKHR& callableShaderBindingTable,
                                const uint32_t width,
                                const uint32_t height,
                                const uint32_t depth) const
  {
    m_commandBuffers[m_currentFrame].traceRaysKHR(
      raygenShaderBindingTable,
      missShaderBindingTable,
      hitShaderBindingTable,
      callableShaderBindingTable,
      width,
      height,
      depth
    );
  }

  void CommandBuffer::copyImage(const vk::Image& srcImage,
                                const vk::ImageLayout srcImageLayout,
                                const vk::Image& dstImage,
                                const vk::ImageLayout dstImageLayout,
                                const std::vector<vk::ImageCopy>& regions) const
  {
    m_commandBuffers[m_currentFrame].copyImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regions);
  }

  void CommandBuffer::allocateCommandBuffers(const vk::CommandPool& commandPool)
  {
    const vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = m_logicalDevice->getMaxFramesInFlight()
    };

    m_logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers);

    m_logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers);
  }

} // namespace vke