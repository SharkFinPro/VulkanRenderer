#include "ImGuiInstance.h"

#include "../components/Window.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/instance/Instance.h"
#include "../core/physicalDevice/PhysicalDevice.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../pipelines/RenderPass.h"
#include "../pipelines/custom/GuiPipeline.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <imgui_internal.h>

ImGuiInstance::ImGuiInstance(const std::shared_ptr<Window>& window,
                             const std::shared_ptr<Instance>& instance,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const std::unique_ptr<GuiPipeline>& guiPipeline,
                             const bool useDockSpace)
  : m_useDockSpace(useDockSpace)
{
  ImGui::CreateContext();

  window->initImGui();

  const SwapChainSupportDetails swapChainSupport = logicalDevice->getPhysicalDevice()->getSwapChainSupport();

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  ImGui_ImplVulkan_InitInfo initInfo {
    .Instance = instance->m_instance,
    .PhysicalDevice = logicalDevice->getPhysicalDevice()->m_physicalDevice,
    .Device = logicalDevice->m_device,
    .Queue = logicalDevice->getGraphicsQueue(),
    .DescriptorPool = guiPipeline->getPool(),
    .RenderPass = renderPass->getRenderPass(),
    .MinImageCount = imageCount,
    .ImageCount = imageCount,
    .MSAASamples = logicalDevice->getPhysicalDevice()->getMsaaSamples()
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

  if (!m_useDockSpace)
  {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::SetNextWindowBgAlpha(1.0f);

  const ImGuiID id = ImGui::GetID("WindowDockSpace");
  ImGui::DockBuilderRemoveNode(id); // Clear previous layout if any
  ImGui::DockBuilderAddNode(id);    // Create new dock node

  if (ImGui::Begin("WindowDockSpace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
  {
    const ImGuiID dockspaceID = ImGui::GetID("WindowDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (m_dockNeedsUpdate)
    {
      // Rebuild the dock layout with current percentages
      ImGui::DockBuilderRemoveNode(dockspaceID);
      ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

      m_mainDock = dockspaceID;

      // Split nodes using current percentages
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Left, m_leftDockPercent, &m_leftDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Right, m_rightDockPercent, &m_rightDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Up, m_topDockPercent, &m_topDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Down, m_bottomDockPercent, &m_bottomDock, &m_mainDock);

      m_centerDock = m_mainDock;

      ImGui::DockBuilderFinish(dockspaceID);
      m_dockNeedsUpdate = false;
    }
  }
  ImGui::End();
}

void ImGuiInstance::dockTop(const char* widget) const
{
  if (!m_mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, m_topDock);
}

void ImGuiInstance::dockBottom(const char* widget) const
{
  if (!m_mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, m_bottomDock);
}

void ImGuiInstance::dockLeft(const char* widget) const
{
  if (!m_mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, m_leftDock);
}

void ImGuiInstance::dockRight(const char* widget) const
{
  if (!m_mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, m_rightDock);
}

void ImGuiInstance::dockCenter(const char* widget) const
{
  if (!m_mainDock)
  {
    return;
  }

  ImGui::DockBuilderDockWindow(widget, m_centerDock);
}

void ImGuiInstance::setTopDockPercent(const float percent)
{
  if (m_topDockPercent == percent)
  {
    return;
  }

  m_topDockPercent = percent;

  m_dockNeedsUpdate = true;
}

void ImGuiInstance::setBottomDockPercent(const float percent)
{
  if (m_bottomDockPercent == percent)
  {
    return;
  }

  m_bottomDockPercent = percent;

  m_dockNeedsUpdate = true;
}

void ImGuiInstance::setLeftDockPercent(const float percent)
{
  if (m_leftDockPercent == percent)
  {
    return;
  }

  m_leftDockPercent = percent;

  m_dockNeedsUpdate = true;
}

void ImGuiInstance::setRightDockPercent(const float percent)
{
  if (m_rightDockPercent == percent)
  {
    return;
  }

  m_rightDockPercent = percent;

  m_dockNeedsUpdate = true;
}

void ImGuiInstance::renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer)
{
  ImGui_ImplVulkan_RenderDrawData(
    ImGui::GetDrawData(),
    commandBuffer->m_commandBuffers[commandBuffer->m_currentFrame],
    nullptr
  );
}
