#ifndef VKE_LINEPIPELINE_H
#define VKE_LINEPIPELINE_H

#include "vertexInputs/LineVertex.h"
#include "../GraphicsPipeline.h"
#include <vector>
#include <memory>

namespace vke {

  class LinePipeline final : public GraphicsPipeline {
  public:
    explicit LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice);

    void render(const std::shared_ptr<LogicalDevice>& logicalDevice,
                const RenderInfo* renderInfo,
                const vk::raii::CommandPool& commandPool,
                const std::vector<LineVertex>* vertices) const;

  private:
    vk::raii::Buffer m_vertexBuffer = nullptr;
    vk::raii::DeviceMemory m_vertexBufferMemory = nullptr;
    size_t m_maxVertexBufferSize = sizeof(LineVertex) * 20'000;

    vk::raii::Buffer m_stagingBuffer = nullptr;
    vk::raii::DeviceMemory m_stagingBufferMemory = nullptr;

    void createVertexBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice);
  };

} // namespace vke

#endif //VKE_LINEPIPELINE_H
