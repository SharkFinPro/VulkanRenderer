#ifndef VKE_MOUSEPICKER_H
#define VKE_MOUSEPICKER_H

#include <imgui.h>
#include <vulkan/vulkan.h>
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
  class Window;

  class MousePicker {
  public:
    MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                std::shared_ptr<Window> window,
                const VkCommandPool& commandPool,
                VkDescriptorSetLayout objectDescriptorSetLayout);

    ~MousePicker();

    void clearObjectsToMousePick();

    void recreateFramebuffer(VkExtent2D viewportExtent);

    void handleRenderedMousePickingImage(VkImage image,
                                         std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender);

    [[nodiscard]] bool canMousePick() const;

    void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked);

    void setViewportPos(ImVec2 viewportPos);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;
    std::shared_ptr<Window> m_window;

    VkExtent2D m_viewportExtent { 1, 1 };

    ImVec2 m_viewportPos {0, 0};

    std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_renderObjectsToMousePick;
    std::unordered_map<uint32_t, bool*> m_mousePickingItems;

    bool m_canMousePick = false;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingBufferMemory = VK_NULL_HANDLE;

    bool validateMousePickingMousePosition(int32_t& mouseX,
                                           int32_t& mouseY);

    [[nodiscard]] uint32_t getIDFromMousePickingImage(VkImage image,
                                                      int32_t mouseX,
                                                      int32_t mouseY) const;

    static void transitionImageForReading(VkCommandBuffer commandBuffer,
                                          VkImage image);

    static void transitionImageForWriting(VkCommandBuffer commandBuffer,
                                          VkImage image);

    [[nodiscard]] uint32_t getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const;

    void handleMousePickingResult(uint32_t objectID,
                                  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender);
  };

} // namespace vke

#endif //VKE_MOUSEPICKER_H
