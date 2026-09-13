#ifndef VKE_IMGUIINSTANCE_H
#define VKE_IMGUIINSTANCE_H

#include "../../EngineConfig.h"
#include "../../utilities/EventSystem.h"
#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>

struct ImGuiDockNode;

namespace vke {

  class CommandBuffer;
  struct ContentScaleEvent;
  class Window;
  class Instance;
  class LogicalDevice;

  class ImGuiInstance {
  public:
    ImGuiInstance(const std::shared_ptr<Window>& window,
                  const std::shared_ptr<Instance>& instance,
                  const std::shared_ptr<LogicalDevice>& logicalDevice,
                  vk::Format swapchainImageFormat,
                  const EngineConfig::ImGui& config);

    ~ImGuiInstance();

    void createNewFrame();

    void dockTop(const char* widget) const;

    void dockBottom(const char* widget) const;

    void dockLeft(const char* widget) const;

    void dockRight(const char* widget) const;

    void dockCenter(const char* widget) const;

    void setTopDockPercent(float percent);

    void setBottomDockPercent(float percent);

    void setLeftDockPercent(float percent);

    void setRightDockPercent(float percent);

    // Docking discards ImGui size constraints, so this floor is enforced on the node holding the window inside the
    // engine's dockspace, and every tab's floor in that node applies. The widget is matched by ImGui ID like dockTop(),
    // so "Label##id" must be passed in full. Size is in unscaled pixels; a zero size removes the floor.
    void setDockedWindowMinimumSize(const char* widget,
                                    ImVec2 minimumSize);

    static ImGuiContext* getImGuiContext();

    void setMenuBarHeight(float height);

    static void render(const std::shared_ptr<CommandBuffer>& commandBuffer);

  private:
    std::shared_ptr<Window> m_window;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    ImGuiStyle m_baseStyle;

    bool m_dockNeedsUpdate = true;

    bool m_useDockSpace;

    // Must match the color attachment format of the swapchain pass ImGui renders into.
    // Stored as a member because the ImGui Vulkan backend reads it again when it creates the first detached window.
    VkFormat m_swapchainColorFormat = VK_FORMAT_UNDEFINED;

    float m_topDockPercent = 0.15f;
    float m_bottomDockPercent = 0.2f;
    float m_leftDockPercent = 0.3f;
    float m_rightDockPercent = 0.3f;

    ImGuiID m_mainDock = 0;
    ImGuiID m_topDock = 0;
    ImGuiID m_bottomDock = 0;
    ImGuiID m_leftDock = 0;
    ImGuiID m_rightDock = 0;
    ImGuiID m_centerDock = 0;

    float m_menuBarHeight = 0.0f;

    std::unordered_map<ImGuiID, ImVec2> m_dockedWindowMinimumSizes;

    EventListener<ContentScaleEvent> m_contentScaleEventListener;

    void createDescriptorPool(const std::shared_ptr<LogicalDevice>& logicalDevice,
                              uint32_t maxImGuiTextures);

    void markDockNeedsUpdate();

    void initFromWindow();

    void displayDockSpace();

    void enforceDockedWindowMinimumSizes(ImGuiID dockSpaceID) const;

    void enforceDockNodeMinimumSize(ImGuiDockNode* node) const;

    static void layoutDockNodeSubtree(ImGuiDockNode* node,
                                      ImVec2 pos,
                                      ImVec2 size);

    [[nodiscard]] ImVec2 getDockNodeMinimumSize(const ImGuiDockNode* node) const;

    static void renderPlatformWindows();

    static void renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer);
  };

} // namespace vke

#endif //VKE_IMGUIINSTANCE_H
