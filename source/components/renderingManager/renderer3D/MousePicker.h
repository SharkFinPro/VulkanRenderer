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

  class LogicalDevice;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;
  class SingleUseCommandBuffer;

  class MousePicker {
  public:
    MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                vk::CommandPool commandPool);

    // Whether the cursor was over the scene image in the most recently rendered frame, whether or not anything pickable
    // was submitted. False after a frame that skipped picking.
    [[nodiscard]] bool canMousePick() const;

    // What the cursor was over in the most recently rendered frame: nullopt over empty space, off the scene, in a frame
    // that skipped picking, or once the object has been destroyed. Objects submitted without a flag are picked too.
    [[nodiscard]] std::optional<PickResult> getPickResult() const;

    // Called before each frame's rendering, so a frame that never reaches the picking readback (a zero-sized scene
    // view, a frame abandoned for a swapchain rebuild) reports that it can't pick rather than the previous answer.
    void beginFrame();

    void clearObjectsToMousePick();

    void setViewportExtent(vk::Extent2D viewportExtent);

    void setViewportPos(ImVec2 viewportPos);

    // The size the scene image is displayed at, in the same ImGui screen coordinates as its position and the cursor.
    // It differs from the viewport extent (the picking image's pixels) wherever window coordinates aren't pixels.
    void setViewportDisplaySize(ImVec2 viewportDisplaySize);

    // Whether the scene view is the UI element under the cursor, rather than covered by another window.
    void setSceneHovered(bool sceneHovered);

    // mousePicked may be null; otherwise it is set to whether the object is under the cursor.
    void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void handleRenderedMousePickingImage(vk::Image image);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::Extent2D m_viewportExtent { 1, 1 };

    ImVec2 m_viewportPos {0, 0};

    ImVec2 m_viewportDisplaySize {0, 0};

    bool m_sceneHovered = false;

    std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_renderObjectsToMousePick;
    // Indexed by object id - 1; entries may be null.
    std::vector<bool*> m_mousePickingFlags;

    // The matrices the picking pass used, for turning a depth back into a world position.
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

    vk::CommandPool m_commandPool = nullptr;

    vk::raii::Buffer m_stagingBuffer = nullptr;
    vk::raii::DeviceMemory m_stagingBufferMemory = nullptr;

    bool validateMousePickingMousePosition(int32_t& mouseX,
                                           int32_t& mouseY);

    [[nodiscard]] std::array<uint32_t, 4> getPixelFromMousePickingImage(vk::Image image,
                                                                        int32_t mouseX,
                                                                        int32_t mouseY) const;

    [[nodiscard]] static std::array<uint32_t, 4> getPixelFromBuffer(const vk::raii::DeviceMemory& stagingBufferMemory);

    [[nodiscard]] glm::vec3 getWorldPosition(int32_t mouseX,
                                             int32_t mouseY,
                                             float depth) const;

    static void transitionImageForReading(const SingleUseCommandBuffer& commandBuffer,
                                          vk::Image image);

    static void transitionImageForWriting(const SingleUseCommandBuffer& commandBuffer,
                                          vk::Image image);
  };

} // namespace vke

#endif //VKE_MOUSEPICKER_H
