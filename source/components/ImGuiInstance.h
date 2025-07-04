#ifndef VULKANPROJECT_IMGUIINSTANCE_H
#define VULKANPROJECT_IMGUIINSTANCE_H

#include <memory>
#include <imgui.h>

class CommandBuffer;
class Window;
class Instance;
class PhysicalDevice;
class LogicalDevice;
class RenderPass;
class GuiPipeline;

class ImGuiInstance {
public:
  ImGuiInstance(const std::shared_ptr<Window>& window,
                const std::shared_ptr<Instance>& instance,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                const std::unique_ptr<GuiPipeline>& guiPipeline,
                bool useDockSpace);
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

  static void renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer);

private:
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
};


#endif //VULKANPROJECT_IMGUIINSTANCE_H
