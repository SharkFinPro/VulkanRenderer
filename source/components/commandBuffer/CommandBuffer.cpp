#include "CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include <stdexcept>

namespace vke {

  CommandBuffer::CommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                               VkCommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    allocateCommandBuffers(commandPool);
  }

  void CommandBuffer::setCurrentFrame(const uint32_t currentFrame)
  {
    m_currentFrame = currentFrame;
  }

  void CommandBuffer::record(const std::function<void()>& renderFunction) const
  {
    constexpr VkCommandBufferBeginInfo beginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    renderFunction();

    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to record command buffer!");
    }
  }

  void CommandBuffer::resetCommandBuffer() const
  {
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
  }

  VkCommandBuffer* CommandBuffer::getCommandBuffer()
  {
    return &m_commandBuffers[m_currentFrame];
  }

  void CommandBuffer::setViewport(const VkViewport& viewport) const
  {
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &viewport);
  }

  void CommandBuffer::setScissor(const VkRect2D& scissor) const
  {
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);
  }

  void CommandBuffer::beginRenderPass(const VkRenderPassBeginInfo& renderPassBeginInfo) const
  {
    vkCmdBeginRenderPass(m_commandBuffers[m_currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  }

  void CommandBuffer::endRenderPass() const
  {
    vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);
  }

  void CommandBuffer::beginRendering(const VkRenderingInfo& renderingInfo) const
  {
    vkCmdBeginRendering(m_commandBuffers[m_currentFrame], &renderingInfo);
  }

  void CommandBuffer::endRendering() const
  {
    vkCmdEndRendering(m_commandBuffers[m_currentFrame]);
  }

  void CommandBuffer::bindPipeline(const VkPipelineBindPoint pipelineBindPoint,
                                   VkPipeline pipeline) const
  {
    vkCmdBindPipeline(m_commandBuffers[m_currentFrame], pipelineBindPoint, pipeline);
  }

  void CommandBuffer::bindDescriptorSets(const VkPipelineBindPoint pipelineBindPoint,
                                         VkPipelineLayout pipelineLayout,
                                         const uint32_t firstSet,
                                         const uint32_t descriptorSetCount,
                                         const VkDescriptorSet* descriptorSets) const
  {
    vkCmdBindDescriptorSets(m_commandBuffers[m_currentFrame], pipelineBindPoint, pipelineLayout, firstSet,
                            descriptorSetCount, descriptorSets, 0, nullptr);
  }

  void CommandBuffer::dispatch(const uint32_t groupCountX,
                               const uint32_t groupCountY,
                               const uint32_t groupCountZ) const
  {
    vkCmdDispatch(m_commandBuffers[m_currentFrame], groupCountX, groupCountY, groupCountZ);
  }

  void CommandBuffer::bindVertexBuffers(const uint32_t firstBinding,
                                        const uint32_t bindingCount,
                                        const VkBuffer* buffers,
                                        const VkDeviceSize* offsets) const
  {
    vkCmdBindVertexBuffers(m_commandBuffers[m_currentFrame], firstBinding, bindingCount, buffers, offsets);
  }

  void CommandBuffer::bindIndexBuffer(VkBuffer buffer,
                                      const VkDeviceSize offset,
                                      const VkIndexType indexType) const
  {
    vkCmdBindIndexBuffer(m_commandBuffers[m_currentFrame], buffer, offset, indexType);
  }

  void CommandBuffer::draw(const uint32_t vertexCount,
                           const uint32_t instanceCount,
                           const uint32_t firstVertex,
                           const uint32_t firstInstance) const
  {
    vkCmdDraw(m_commandBuffers[m_currentFrame], vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void CommandBuffer::drawIndexed(const uint32_t indexCount,
                                  const uint32_t instanceCount,
                                  const uint32_t firstIndex,
                                  const int32_t vertexOffset,
                                  const uint32_t firstInstance) const
  {
    vkCmdDrawIndexed(m_commandBuffers[m_currentFrame], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  }

  void CommandBuffer::pushConstants(const VkPipelineLayout layout,
                                    const VkShaderStageFlags stageFlags,
                                    const uint32_t offset,
                                    const uint32_t size,
                                    const void* values) const
  {
    vkCmdPushConstants(m_commandBuffers[m_currentFrame], layout, stageFlags, offset, size, values);
  }

  void CommandBuffer::pipelineBarrier(const VkPipelineStageFlags srcStageMask,
                                      const VkPipelineStageFlags dstStageMask,
                                      const VkDependencyFlags dependencyFlags,
                                      const std::vector<VkMemoryBarrier>& memoryBarriers,
                                      const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
                                      const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers) const
  {
    vkCmdPipelineBarrier(
      m_commandBuffers[m_currentFrame],
      srcStageMask,
      dstStageMask,
      dependencyFlags,
      memoryBarriers.size(),
      memoryBarriers.data(),
      bufferMemoryBarriers.size(),
      bufferMemoryBarriers.data(),
      imageMemoryBarriers.size(),
      imageMemoryBarriers.data()
    );
  }

  void CommandBuffer::clearAttachments(const std::vector<VkClearAttachment>& clearAttachments,
                                       const std::vector<VkClearRect>& clearRects) const
  {
    vkCmdClearAttachments(
      m_commandBuffers[m_currentFrame],
      clearAttachments.size(),
      clearAttachments.data(),
      clearRects.size(),
      clearRects.data()
    );
  }

  void CommandBuffer::allocateCommandBuffers(VkCommandPool commandPool)
  {
    m_commandBuffers.resize(m_logicalDevice->getMaxFramesInFlight());

    const VkCommandBufferAllocateInfo allocInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
    };

    m_logicalDevice->allocateCommandBuffers(allocInfo, m_commandBuffers.data());
  }

} // namespace vke