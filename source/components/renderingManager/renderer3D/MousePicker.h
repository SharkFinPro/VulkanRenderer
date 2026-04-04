#ifndef VKE_MOUSEPICKER_H
#define VKE_MOUSEPICKER_H

#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class CommandBuffer;
  class Framebuffer;
  class LogicalDevice;
  class PipelineManager;
  enum class PipelineType;
  struct RenderInfo;
  class RenderObject;
  class RenderPass;
  class RenderTarget;
  class SingleUseCommandBuffer;
  class Window;

  class MousePicker {
  public:
    MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                std::shared_ptr<Window> window,
                vk::CommandPool commandPool);

    [[nodiscard]] bool canMousePick() const;

    void clearObjectsToMousePick();

    void setViewportExtent(vk::Extent2D viewportExtent);

    void setViewportPos(ImVec2 viewportPos);

    void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void handleRenderedMousePickingImage(vk::Image image);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;
    std::shared_ptr<Window> m_window;

    vk::Extent2D m_viewportExtent { 1, 1 };

    ImVec2 m_viewportPos {0, 0};

    std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_renderObjectsToMousePick;
    std::unordered_map<uint32_t, bool*> m_mousePickingItems;

    bool m_canMousePick = false;

    vk::CommandPool m_commandPool = VK_NULL_HANDLE;

    vk::raii::Buffer m_stagingBuffer = VK_NULL_HANDLE;
    vk::raii::DeviceMemory m_stagingBufferMemory = VK_NULL_HANDLE;

    bool validateMousePickingMousePosition(int32_t& mouseX,
                                           int32_t& mouseY);

    [[nodiscard]] uint32_t getIDFromMousePickingImage(vk::Image image,
                                                      int32_t mouseX,
                                                      int32_t mouseY) const;

    [[nodiscard]] uint32_t getObjectIDFromBuffer(const vk::raii::DeviceMemory& stagingBufferMemory) const;

    static void transitionImageForReading(const SingleUseCommandBuffer& commandBuffer,
                                          vk::Image image);

    static void transitionImageForWriting(const SingleUseCommandBuffer& commandBuffer,
                                          vk::Image image);
  };

} // namespace vke

#endif //VKE_MOUSEPICKER_H
