#ifndef LINEPIPELINE_H
#define LINEPIPELINE_H

#include "../GraphicsPipeline.h"
#include "vertexInputs/LineVertex.h"
#include <vector>
#include <memory>

class DescriptorSet;
class RenderPass;

class LinePipeline final : public GraphicsPipeline {
public:
  LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
               std::shared_ptr<RenderPass> renderPass);

  ~LinePipeline() override;

  void render(const RenderInfo* renderInfo, const VkCommandPool& commandPool, const std::vector<LineVertex>& vertices);

private:
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
  size_t m_maxVertexBufferSize = sizeof(LineVertex) * 20'000;

  VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_stagingBufferMemory = VK_NULL_HANDLE;

  void createVertexBuffer();
};



#endif //LINEPIPELINE_H
