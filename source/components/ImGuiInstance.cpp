#include "ImGuiInstance.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "../utilities/Buffers.h"

#include "../components/Window.h"
#include "../components/Instance.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include "../pipelines/RenderPass.h"
#include "../pipelines/custom/GuiPipeline.h"

ImGuiInstance::ImGuiInstance(const std::shared_ptr<Window>& window,
                             const std::shared_ptr<Instance>& instance,
                             const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const std::unique_ptr<GuiPipeline>& guiPipeline,
                             const bool useDockSpace)
  : useDockSpace(useDockSpace)
{
  ImGui::CreateContext();

  window->initImGui();

  const SwapChainSupportDetails swapChainSupport = physicalDevice->getSwapChainSupport();

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  ImGui_ImplVulkan_InitInfo initInfo {
    .Instance = instance->getInstance(),
    .PhysicalDevice = physicalDevice->getPhysicalDevice(),
    .Device = logicalDevice->getDevice(),
    .Queue = logicalDevice->getGraphicsQueue(),
    .DescriptorPool = guiPipeline->getPool(),
    .RenderPass = renderPass->getRenderPass(),
    .MinImageCount = imageCount,
    .ImageCount = imageCount,
    .MSAASamples = physicalDevice->getMsaaSamples()
  };

  ImGui_ImplVulkan_Init(&initInfo);

  if (useDockSpace)
  {
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  }

  createNewFrame();
}

ImGuiInstance::~ImGuiInstance()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiInstance::createNewFrame()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (!useDockSpace)
  {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::SetNextWindowBgAlpha(1.0f);

  ImGuiID id = ImGui::GetID("WindowDockSpace");
  ImGui::DockBuilderRemoveNode(id); // Clear previous layout if any
  ImGui::DockBuilderAddNode(id);    // Create new dock node

  if (ImGui::Begin("WindowDockSpace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
  {
    const ImGuiID dockspaceID = ImGui::GetID("WindowDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (dockNeedsUpdate)
    {
      // Rebuild the dock layout with current percentages
      ImGui::DockBuilderRemoveNode(dockspaceID);
      ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

      mainDock = dockspaceID;

      // Split nodes using current percentages
      ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Left, leftDockPercent, &leftDock, &mainDock);
      ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Right, rightDockPercent, &rightDock, &mainDock);
      ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Up, topDockPercent, &topDock, &mainDock);
      ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Down, bottomDockPercent, &bottomDock, &mainDock);

      centerDock = mainDock;

      ImGui::DockBuilderFinish(dockspaceID);
      dockNeedsUpdate = false;
    }
  }
  ImGui::End();
}

void ImGuiInstance::dockTop(const char* widget) const
{
  if (!mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, topDock);
}

void ImGuiInstance::dockBottom(const char* widget) const
{
  if (!mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, bottomDock);
}

void ImGuiInstance::dockLeft(const char* widget) const
{
  if (!mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, leftDock);
}

void ImGuiInstance::dockRight(const char* widget) const
{
  if (!mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, rightDock);
}

void ImGuiInstance::dockCenter(const char* widget) const
{
  if (!mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, centerDock);
}

void ImGuiInstance::setTopDockPercent(const float percent)
{
  if (topDockPercent == percent)
  {
    return;
  }

  topDockPercent = percent;

  dockNeedsUpdate = true;
}

void ImGuiInstance::setBottomDockPercent(const float percent)
{
  if (bottomDockPercent == percent)
  {
    return;
  }

  bottomDockPercent = percent;

  dockNeedsUpdate = true;
}

void ImGuiInstance::setLeftDockPercent(const float percent)
{
  if (leftDockPercent == percent)
  {
    return;
  }

  leftDockPercent = percent;

  dockNeedsUpdate = true;
}

void ImGuiInstance::setRightDockPercent(const float percent)
{
  if (rightDockPercent == percent)
  {
    return;
  }

  rightDockPercent = percent;

  dockNeedsUpdate = true;
}