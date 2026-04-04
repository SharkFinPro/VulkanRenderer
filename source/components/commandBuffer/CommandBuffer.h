#ifndef VKE_COMMANDBUFFER_H
#define VKE_COMMANDBUFFER_H

#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class CommandBuffer {
  public:
    explicit CommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice);

    CommandBuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                  vk::CommandPool& commandPool);

    virtual ~CommandBuffer() = default;

    void setCurrentFrame(uint32_t currentFrame);

    virtual void record(const std::function<void()>& renderFunction) const;

    void resetCommandBuffer() const;

    [[nodiscard]] vk::CommandBuffer getCommandBuffer();

    void setViewport(const vk::Viewport& viewport) const;

    void setScissor(const vk::Rect2D& scissor) const;

    void beginRenderPass(const vk::RenderPassBeginInfo& renderPassBeginInfo) const;

    void endRenderPass() const;

    void beginRendering(const vk::RenderingInfo& renderingInfo) const;

    void endRendering() const;

    void bindPipeline(vk::PipelineBindPoint pipelineBindPoint,
                      const vk::Pipeline& pipeline) const;

    void bindDescriptorSets(vk::PipelineBindPoint pipelineBindPoint,
                            const vk::PipelineLayout& pipelineLayout,
                            uint32_t firstSet,
                            const std::vector<vk::DescriptorSet>& descriptorSets) const;

    void dispatch(uint32_t groupCountX,
                  uint32_t groupCountY,
                  uint32_t groupCountZ) const;

    void bindVertexBuffers(uint32_t firstBinding,
                           const std::vector<vk::Buffer>& buffers,
                           const std::vector<vk::DeviceSize>& offsets) const;

    void bindIndexBuffer(const vk::Buffer& buffer,
                         vk::DeviceSize offset,
                         vk::IndexType indexType) const;

    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) const;

    void drawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t firstIndex,
                     int32_t vertexOffset,
                     uint32_t firstInstance) const;

    template<typename T>
    void pushConstants(const vk::PipelineLayout& layout,
                       vk::ShaderStageFlags stageFlags,
                       uint32_t offset,
                       const T& data) const;

    void pipelineBarrier(vk::PipelineStageFlags srcStageMask,
                         vk::PipelineStageFlags dstStageMask,
                         vk::DependencyFlags dependencyFlags,
                         const std::vector<vk::MemoryBarrier>& memoryBarriers,
                         const std::vector<vk::BufferMemoryBarrier>& bufferMemoryBarriers,
                         const std::vector<vk::ImageMemoryBarrier>& imageMemoryBarriers) const;

    void clearAttachments(const std::vector<vk::ClearAttachment>& clearAttachments,
                          const std::vector<vk::ClearRect>& clearRects) const;

    void copyImageToBuffer(const vk::Image& srcImage,
                           vk::ImageLayout srcImageLayout,
                           const vk::Buffer& dstBuffer,
                           const std::vector<vk::BufferImageCopy>& regions) const;

    void copyBufferToImage(const vk::Buffer& srcBuffer,
                           const vk::Image& dstImage,
                           vk::ImageLayout dstImageLayout,
                           const std::vector<vk::BufferImageCopy>& regions) const;

    void blitImage(const vk::Image& srcImage,
                   vk::ImageLayout srcImageLayout,
                   const vk::Image& dstImage,
                   vk::ImageLayout dstImageLayout,
                   const std::vector<vk::ImageBlit>& regions,
                   vk::Filter filter) const;

    void copyBuffer(const vk::Buffer& srcBuffer,
                    const vk::Buffer& dstBuffer,
                    const std::vector<vk::BufferCopy>& regions) const;

    void buildAccelerationStructure(const vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                                    const vk::AccelerationStructureBuildRangeInfoKHR* buildRangeInfo) const;

    void traceRays(const vk::StridedDeviceAddressRegionKHR& raygenShaderBindingTable,
                   const vk::StridedDeviceAddressRegionKHR& missShaderBindingTable,
                   const vk::StridedDeviceAddressRegionKHR& hitShaderBindingTable,
                   const vk::StridedDeviceAddressRegionKHR& callableShaderBindingTable,
                   uint32_t width,
                   uint32_t height,
                   uint32_t depth) const;

    void copyImage(const vk::Image& srcImage,
                   vk::ImageLayout srcImageLayout,
                   const vk::Image& dstImage,
                   vk::ImageLayout dstImageLayout,
                   const std::vector<vk::ImageCopy>& regions) const;

    friend class ImGuiInstance;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<vk::raii::CommandBuffer> m_commandBuffers;

    uint32_t m_currentFrame = 0;

    virtual void allocateCommandBuffers(const vk::CommandPool& commandPool);
  };

  template<typename T>
  void CommandBuffer::pushConstants(const vk::PipelineLayout& layout,
                                    vk::ShaderStageFlags stageFlags,
                                    uint32_t offset,
                                    const T& data) const
  {
    m_commandBuffers[m_currentFrame].pushConstants(layout, stageFlags, offset, data);
  }
} // namespace vke

#endif //VKE_COMMANDBUFFER_H