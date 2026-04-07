#ifndef VKE_IMGUIINSTANCE_H
#define VKE_IMGUIINSTANCE_H

#include "../../EngineConfig.h"
#include "../../utilities/EventSystem.h"
#include <imgui.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>

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

    static ImGuiContext* getImGuiContext();

    void setMenuBarHeight(float height);

    static void render(const std::shared_ptr<CommandBuffer>& commandBuffer);

  private:
    std::shared_ptr<Window> m_window;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    ImGuiStyle m_baseStyle;

    bool m_dockNeedsUpdate = true;

    bool m_useDockSpace;

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

    EventListener<ContentScaleEvent> m_contentScaleEventListener;

    void createDescriptorPool(const std::shared_ptr<LogicalDevice>& logicalDevice,
                              uint32_t maxImGuiTextures);

    void markDockNeedsUpdate();

    void initFromWindow();

    void displayDockSpace();

    static void renderPlatformWindows();

    static void renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer);
  };

} // namespace vke

#endif //VKE_IMGUIINSTANCE_H
