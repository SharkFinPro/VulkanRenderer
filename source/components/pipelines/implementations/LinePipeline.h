#ifndef VKE_LINEPIPELINE_H
#define VKE_LINEPIPELINE_H

#include "vertexInputs/LineVertex.h"
#include "../GraphicsPipeline.h"
#include <vector>
#include <memory>

namespace vke {

  class DescriptorSet;
  class RenderPass;

  class LinePipeline final : public GraphicsPipeline {
  public:
    LinePipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 const std::shared_ptr<RenderPass>& renderPass);

    void render(const RenderInfo* renderInfo,
                const vk::raii::CommandPool& commandPool,
                const std::vector<LineVertex>* vertices) const;

  private:
    vk::raii::Buffer m_vertexBuffer = nullptr;
    vk::raii::DeviceMemory m_vertexBufferMemory = nullptr;
    size_t m_maxVertexBufferSize = sizeof(LineVertex) * 20'000;

    vk::raii::Buffer m_stagingBuffer = nullptr;
    vk::raii::DeviceMemory m_stagingBufferMemory = nullptr;

    void createVertexBuffer();
  };

} // namespace vke

#endif //VKE_LINEPIPELINE_H
