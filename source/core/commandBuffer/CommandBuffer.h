#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <vector>

class LogicalDevice;

class CommandBuffer {
public:
  CommandBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

  void setCurrentFrame(uint32_t currentFrame);

  void record(const std::function<void()>& renderFunction) const;

  void resetCommandBuffer() const;

  [[nodiscard]] VkCommandBuffer* getCommandBuffer();

  void setViewport(const VkViewport& viewport) const;

  void setScissor(const VkRect2D& scissor) const;

  void beginRenderPass(const VkRenderPassBeginInfo& renderPassBeginInfo) const;

  void endRenderPass() const;

  void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const;

  void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout pipelineLayout, uint32_t firstSet,
                          uint32_t descriptorSetCount, const VkDescriptorSet* descriptorSets) const;

  void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;

  void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers,
                         const VkDeviceSize* offsets) const;

  void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const;

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

  void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance) const;

  void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                     const void* values) const;

  friend class ImGuiInstance;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::vector<VkCommandBuffer> m_commandBuffers;

  uint32_t m_currentFrame = 0;

  void allocateCommandBuffers(VkCommandPool commandPool);
};



#endif //COMMANDBUFFER_H
