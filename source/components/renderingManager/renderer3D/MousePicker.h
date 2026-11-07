#ifndef VKE_MOUSEPICKER_H
#define VKE_MOUSEPICKER_H

#include "PickResult.h"
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;

  class MousePicker {
  public:
    explicit MousePicker(std::shared_ptr<LogicalDevice> logicalDevice);

    // Whether the cursor was over the scene image in the most recently rendered frame, whether or not anything pickable
    // was submitted. False after a frame that skipped picking. It describes a newer frame than getPickResult, so a pick
    // can still be reported for a moment after this turns false.
    [[nodiscard]] bool canMousePick() const;

    // What the cursor was over in a frame rendered a few frames ago: nullopt over empty space, off the scene, when no
    // result has arrived, or once the object has been destroyed. Objects submitted without a flag are picked too. The
    // one-pixel readback is recorded into the frame's command buffer and read when that frame-in-flight slot comes
    // around again, so the answer lags by up to the number of frames in flight and nothing waits on the GPU. Like the
    // flags, it is set while the frame renders, so read it before submitting this frame's objects.
    [[nodiscard]] std::optional<PickResult> getPickResult() const;

    // Called before each frame's rendering, so a frame that never reaches the picking readback (a zero-sized scene
    // view, a frame abandoned for a swapchain rebuild) reports that it can't pick rather than the previous answer.
    void beginFrame();

    // Reads the result recorded when this frame-in-flight slot was last used, once the frame has completed (call after
    // FrameScheduler::beginFrame and beginFrame). Sets the pick result and the flags of this frame's submissions of the
    // picked object.
    void resolveReadback(uint32_t currentFrame);

    void clearObjectsToMousePick();

    void setViewportExtent(vk::Extent2D viewportExtent);

    void setViewportPos(ImVec2 viewportPos);

    // The size the scene image is displayed at, in the same ImGui screen coordinates as its position and the cursor.
    // It differs from the viewport extent (the picking image's pixels) wherever window coordinates aren't pixels.
    void setViewportDisplaySize(ImVec2 viewportDisplaySize);

    // Whether the scene view is the UI element under the cursor, rather than covered by another window.
    void setSceneHovered(bool sceneHovered);

    // mousePicked may be null; otherwise it is set to whether the object was under the cursor in the frame the readback
    // describes (see getPickResult), on every submission of that object this frame. An alpha-tested object is picked
    // only where its texture is opaque enough, matching how it is drawn.
    void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked, bool alphaTested);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    // Records the one-pixel copy under the cursor into the frame's command buffer, after the picking pass has ended.
    // Only the host read after the frame completes uses the result. Also updates canMousePick.
    void recordReadback(const std::shared_ptr<CommandBuffer>& commandBuffer,
                        vk::Image image,
                        uint32_t currentFrame);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::Extent2D m_viewportExtent { 1, 1 };

    ImVec2 m_viewportPos {0, 0};

    ImVec2 m_viewportDisplaySize {0, 0};

    bool m_sceneHovered = false;

    struct PickEntry {
      std::shared_ptr<RenderObject> object;
      uint32_t id;
      bool alphaTested;
    };

    std::vector<PickEntry> m_renderObjectsToMousePick;
    // Indexed by object id - 1; entries may be null.
    std::vector<bool*> m_mousePickingFlags;

    // The matrices the latest picking pass used, for turning a depth back into a world position.
    mutable glm::mat4 m_viewMatrix { 1.0f };
    mutable glm::mat4 m_projectionMatrix { 1.0f };

    struct Pick {
      std::weak_ptr<RenderObject> renderObject;
      uint32_t triangleIndex;
      glm::vec3 worldPosition;
      float depth;
    };
    std::optional<Pick> m_pick;

    bool m_canMousePick = false;

    // One per frame in flight: the readback buffer (persistently mapped) and what is needed to interpret it. The
    // entries are the frame's pick entries, swapped in so nothing is copied; they keep the objects alive for at most
    // the frames in flight after the application stops submitting them. The application's flags are not kept.
    struct ReadbackSlot {
      vk::raii::Buffer buffer = nullptr;
      vk::raii::DeviceMemory memory = nullptr;
      const void* mapped = nullptr;
      bool pending = false;
      int32_t mouseX = 0;
      int32_t mouseY = 0;
      vk::Extent2D extent { 1, 1 };
      glm::mat4 viewMatrix { 1.0f };
      glm::mat4 projectionMatrix { 1.0f };
      std::vector<PickEntry> entries;
    };
    std::vector<ReadbackSlot> m_readbackSlots;

    void createReadbackSlots();

    void renderEntries(const RenderInfo* renderInfo,
                       const std::shared_ptr<PipelineManager>& pipelineManager,
                       bool alphaTested) const;

    bool validateMousePickingMousePosition(int32_t& mouseX,
                                           int32_t& mouseY);

    void storePick(const ReadbackSlot& slot,
                   const std::array<uint32_t, 4>& pixel);

    void flagCurrentSubmissions(const RenderObject* picked);

    [[nodiscard]] static glm::vec3 getWorldPosition(const ReadbackSlot& slot,
                                                    float depth);

    static void recordPixelCopy(const CommandBuffer& commandBuffer,
                                vk::Image image,
                                const ReadbackSlot& slot);

    static void transitionImageForReading(const CommandBuffer& commandBuffer,
                                          vk::Image image);

    static void transitionImageForWriting(const CommandBuffer& commandBuffer,
                                          vk::Image image);

    static void makeReadbackVisibleToHost(const CommandBuffer& commandBuffer,
                                          vk::Buffer buffer);
  };

} // namespace vke

#endif //VKE_MOUSEPICKER_H
