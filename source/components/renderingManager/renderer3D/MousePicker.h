#ifndef VKE_MOUSEPICKER_H
#define VKE_MOUSEPICKER_H

#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;
  class Window;

  // Picking reads back a single pixel of the picking attachment, which the CPU cannot observe
  // until the GPU has finished the frame that produced it. Rather than blocking on that, the
  // copy is recorded into the frame's own offscreen command buffer and the result is read
  // maxFramesInFlight frames later, once the slot comes back around: FrameScheduler::beginFrame()
  // has already waited for that frame to complete, so the read costs nothing.
  //
  // Picking results therefore describe the scene as it was maxFramesInFlight frames ago. Object
  // IDs are assigned in registration order, so a frame that registers a different set of objects
  // than the one being resolved can briefly attribute a hit to the wrong object.
  class MousePicker {
  public:
    MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                std::shared_ptr<Window> window);

    [[nodiscard]] bool canMousePick() const;

    void clearObjectsToMousePick();

    void setViewportExtent(vk::Extent2D viewportExtent);

    void setViewportPos(ImVec2 viewportPos);

    void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    // Applies the result of the readback recorded into this slot the last time it was used, and
    // refreshes canMousePick(). Must run before recordReadback() overwrites the slot.
    void resolveReadback(uint32_t currentFrame);

    // Records the 1x1 copy of the pixel under the cursor into the frame's command buffer, to be
    // picked up by resolveReadback() maxFramesInFlight frames from now.
    void recordReadback(const std::shared_ptr<CommandBuffer>& commandBuffer,
                        uint32_t currentFrame,
                        vk::Image image);

  private:
    // One staging buffer per frame in flight, so that a slot is only ever written by one
    // in-flight frame and only read once that frame has completed.
    struct ReadbackSlot {
      vk::raii::Buffer buffer = nullptr;
      vk::raii::DeviceMemory memory = nullptr;
      void* mapped = nullptr;
      bool pending = false;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;
    std::shared_ptr<Window> m_window;

    vk::Extent2D m_viewportExtent { 1, 1 };

    ImVec2 m_viewportPos {0, 0};

    std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_renderObjectsToMousePick;
    std::unordered_map<uint32_t, bool*> m_mousePickingItems;

    std::vector<ReadbackSlot> m_readbackSlots;

    bool m_canMousePick = false;

    void createReadbackSlots();

    bool validateMousePickingMousePosition(int32_t& mouseX,
                                           int32_t& mouseY);

    [[nodiscard]] static uint32_t getObjectIDFromBuffer(const void* mappedMemory);

    static void transitionImageForReading(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          vk::Image image);

    static void transitionImageForWriting(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          vk::Image image);

    static void barrierForHostRead(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   vk::Buffer buffer);
  };

} // namespace vke

#endif //VKE_MOUSEPICKER_H
