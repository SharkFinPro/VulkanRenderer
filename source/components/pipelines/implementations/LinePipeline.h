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

    void render(const RenderInfo* renderInfo,
                const std::vector<LineVertex>* vertices) const;

  private:
    // One host-visible vertex buffer per frame in flight, persistently mapped. Line geometry is
    // written straight into the current frame's buffer at record time, so there is no staging
    // copy (and its device stall) between recording and drawing.
    std::vector<vk::raii::Buffer> m_vertexBuffers;
    std::vector<vk::raii::DeviceMemory> m_vertexBuffersMemory;
    std::vector<void*> m_vertexBuffersMapped;
    size_t m_maxVertexBufferSize = sizeof(LineVertex) * 20'000;

    void createVertexBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice);
  };

} // namespace vke

#endif //VKE_LINEPIPELINE_H
