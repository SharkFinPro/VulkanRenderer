#ifndef VKE_LINEPIPELINE_H
#define VKE_LINEPIPELINE_H

#include "vertexInputs/LineInstance.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  struct RenderInfo;

  // Draws debug lines in three depth modes that differ only in their depth state (and the hidden x-ray pass's
  // opacity). The modes share one instance buffer per frame, with a slot of s_maxLines for each.
  class LinePipeline final {
  public:
    // Per depth mode
    static constexpr size_t s_maxLines = 10'000;

    // Opacity of the hidden parts of x-ray lines, as a fraction of the line's alpha
    static constexpr float s_hiddenAlphaScale = 0.35f;

    explicit LinePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice);

    // Uploads every mode's lines for the frame, then draws the tested and x-ray ones. The on-top lines are drawn
    // by renderOnTop, so that they can come after everything else in the pass.
    void render(const RenderInfo* renderInfo,
                const LineBatches* lines) const;

    void renderOnTop(const RenderInfo* renderInfo,
                     size_t lineCount) const;

  private:
    GraphicsPipeline m_testedPipeline;
    GraphicsPipeline m_onTopPipeline;
    GraphicsPipeline m_hiddenPipeline;

    // One host-visible instance buffer per frame in flight, persistently mapped. Line data is
    // written straight into the current frame's buffer at record time, so there is no staging
    // copy (and its device stall) between recording and drawing.
    std::vector<vk::raii::Buffer> m_vertexBuffers;
    std::vector<vk::raii::DeviceMemory> m_vertexBuffersMemory;
    std::vector<void*> m_vertexBuffersMapped;
    size_t m_maxVertexBufferSize = sizeof(LineInstance) * s_maxLines * s_debugDepthModeCount;
    mutable bool m_warnedAboutLineLimit = false;

    void createVertexBuffers(const std::shared_ptr<LogicalDevice>& logicalDevice);

    void draw(const RenderInfo* renderInfo,
              const GraphicsPipeline& pipeline,
              DebugDepth slot,
              size_t lineCount,
              float alphaScale) const;
  };

} // namespace vke

#endif //VKE_LINEPIPELINE_H
