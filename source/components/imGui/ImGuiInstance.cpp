#include "ImGuiInstance.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../instance/Instance.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../window/Window.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace vke {

  ImGuiInstance::ImGuiInstance(const std::shared_ptr<Window>& window,
                               const std::shared_ptr<Instance>& instance,
                               const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const vk::Format swapchainImageFormat,
                               const EngineConfig::ImGui& config)
    : m_window(window), m_useDockSpace(config.useDockspace),
      m_swapchainColorFormat(static_cast<VkFormat>(swapchainImageFormat))
  {
    createDescriptorPool(logicalDevice, config.maxTextures);

    ImGui::CreateContext();

    const QueueFamilyIndices queueFamilies = logicalDevice->getPhysicalDevice()->getQueueFamilies();

    if (m_useDockSpace)
    {
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      // The backend presents detached windows on the graphics queue, so they are only enabled when the engine presents
      // from that queue family too.
      if (config.detachableWindows && queueFamilies.graphicsFamily == queueFamilies.presentFamily)
      {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // A detached window can sit on a monitor with a different DPI from the main window. ImGui then sizes each
        // window's text for its own monitor and rescales windows that move between monitors, with the DPI chosen by
        // getViewportDpiScale(). Padding and other style sizes stay at the main window's scale; ImGui can't scale those
        // per monitor yet.
        ImGui::GetIO().ConfigDpiScaleFonts = true;
        ImGui::GetIO().ConfigDpiScaleViewports = true;

        // GLFW keeps window hints, so the windows ImGui creates would inherit the main window's scale-to-monitor hint.
        // Windows would then resize them for DPI on its own, on top of ImGui's rescale, and their size would stop
        // matching the size ImGui draws.
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
      }
    }

    ImGui_ImplGlfw_InitForVulkan(window->getWindow(), true);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGui::GetPlatformIO().Platform_GetWindowDpiScale = getViewportDpiScale;
    }

    if (config.styleSetup)
    {
      config.styleSetup();
    }

    initFromWindow();

    const SwapChainSupportDetails swapChainSupport = logicalDevice->getPhysicalDevice()->getSwapChainSupport();

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    ImGui_ImplVulkan_InitInfo initInfo {
      .Instance = static_cast<VkInstance>(*instance->m_instance),
      .PhysicalDevice = static_cast<VkPhysicalDevice>(*logicalDevice->getPhysicalDevice()->m_physicalDevice),
      .Device = static_cast<VkDevice>(*logicalDevice->m_device),
      .QueueFamily = queueFamilies.graphicsFamily.value(),
      .Queue = static_cast<VkQueue>(logicalDevice->getGraphicsQueue()),
      .DescriptorPool = static_cast<VkDescriptorPool>(*m_descriptorPool),
      .MinImageCount = imageCount,
      .ImageCount = imageCount,
      .PipelineInfoMain {
        .RenderPass = nullptr,
        .MSAASamples = static_cast<VkSampleCountFlagBits>(logicalDevice->getPhysicalDevice()->getMsaaSamples())
      }
    };

    initInfo.UseDynamicRendering = true;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &m_swapchainColorFormat,
      .depthAttachmentFormat = static_cast<VkFormat>(logicalDevice->getPhysicalDevice()->findDepthFormat())
    };

    // Detached windows get their own swapchains from the backend. Asking for the main swapchain's format keeps their
    // colors matching the main window's wherever the surface supports it.
    initInfo.PipelineInfoForViewports.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &m_swapchainColorFormat
    };

    // The backend passes every result here, including successes. Detached windows make it submit and present on its
    // own, so its failures are raised like the engine's instead of being dropped.
    initInfo.CheckVkResultFn = [](const VkResult result) {
      if (result < 0)
      {
        throw std::runtime_error("ImGui Vulkan backend call failed with VkResult " + std::to_string(result));
      }
    };

    ImGui_ImplVulkan_Init(&initInfo);

    createNewFrame();
  }

  ImGuiInstance::~ImGuiInstance()
  {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_window->removeListener(m_contentScaleEventListener);
  }

  void ImGuiInstance::createNewFrame()
  {
    // A frame abandoned before its draws were recorded (an out-of-date swapchain) never reached ImGui::Render(), but
    // ImGui requires every frame to be ended, and its platform windows updated, before the next one begins.
    if (ImGui::GetCurrentContext()->WithinFrameScope)
    {
      ImGui::EndFrame();
      ImGui::UpdatePlatformWindows();
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    displayDockSpace();
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

    markDockNeedsUpdate();
  }

  void ImGuiInstance::setBottomDockPercent(const float percent)
  {
    if (m_bottomDockPercent == percent)
    {
      return;
    }

    m_bottomDockPercent = percent;

    markDockNeedsUpdate();
  }

  void ImGuiInstance::setLeftDockPercent(const float percent)
  {
    if (m_leftDockPercent == percent)
    {
      return;
    }

    m_leftDockPercent = percent;

    markDockNeedsUpdate();
  }

  void ImGuiInstance::setRightDockPercent(const float percent)
  {
    if (m_rightDockPercent == percent)
    {
      return;
    }

    m_rightDockPercent = percent;

    markDockNeedsUpdate();
  }

  void ImGuiInstance::setDockedWindowMinimumSize(const char* widget,
                                                 const ImVec2 minimumSize)
  {
    if (minimumSize.x <= 0.0f && minimumSize.y <= 0.0f)
    {
      m_dockedWindowMinimumSizes.erase(ImHashStr(widget));
      return;
    }

    m_dockedWindowMinimumSizes[ImHashStr(widget)] = minimumSize;
  }

  ImGuiContext* ImGuiInstance::getImGuiContext()
  {
    return ImGui::GetCurrentContext();
  }

  void ImGuiInstance::setMenuBarHeight(const float height)
  {
    m_menuBarHeight = height;
  }

  void ImGuiInstance::render(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    ImGui::Render();

    renderDrawData(commandBuffer);

    // Detached windows are submitted and presented here, before the swapchain pass is submitted. They can sample images
    // the frame's offscreen pass writes, and queue order then keeps them inside the frame the scheduler waits on.
    renderPlatformWindows();
  }

  void ImGuiInstance::createDescriptorPool(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const uint32_t maxImGuiTextures)
  {
    const std::array<vk::DescriptorPoolSize, 2> poolSizes {{
      {vk::DescriptorType::eSampler, logicalDevice->getMaxFramesInFlight() * maxImGuiTextures},
      {vk::DescriptorType::eSampledImage, logicalDevice->getMaxFramesInFlight() * maxImGuiTextures}
    }};

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = logicalDevice->getMaxFramesInFlight() * maxImGuiTextures,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = logicalDevice->createDescriptorPool(poolCreateInfo);
  }

  void ImGuiInstance::markDockNeedsUpdate()
  {
    m_dockNeedsUpdate = true;
  }

  void ImGuiInstance::initFromWindow()
  {
    m_baseStyle = ImGui::GetStyle();

    float xscale, yscale;
    glfwGetWindowContentScale(m_window->getWindow(), &xscale, &yscale);

    applyContentScale(xscale);

    m_contentScaleEventListener = m_window->on<ContentScaleEvent>([this](const ContentScaleEvent& e) {
      applyContentScale(e.xscale);
    });
  }

  void ImGuiInstance::applyContentScale(const float contentScale)
  {
    ImGuiStyle& style = ImGui::GetStyle();
    style = m_baseStyle;
    style.ScaleAllSizes(contentScale);

    if (!ImGui::GetIO().ConfigDpiScaleFonts)
    {
      ImGui::GetIO().FontGlobalScale = contentScale;
      return;
    }

    // ImGui multiplies text by the DPI scale of each window's monitor. That scale is 1 where the platform works in
    // points (macOS), so whatever part of the main window's content scale it doesn't cover goes into the main font scale.
    const float monitorScale = ImGui_ImplGlfw_GetContentScaleForWindow(m_window->getWindow());
    if (monitorScale > 0.0f)
    {
      style.FontScaleMain = m_baseStyle.FontScaleMain * contentScale / monitorScale;
    }
  }

  void ImGuiInstance::displayDockSpace()
  {
    if (!m_useDockSpace)
    {
      return;
    }

    const ImGuiID dockSpaceID = ImGui::GetID("WindowDockSpace");
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (ImGui::DockBuilderGetNode(dockSpaceID) == nullptr || m_dockNeedsUpdate)
    {
      // Rebuild the dock layout with current percentages
      ImGui::DockBuilderRemoveNode(dockSpaceID);
      ImGui::DockBuilderAddNode(dockSpaceID, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockSpaceID, viewport->Size);

      m_mainDock = dockSpaceID;

      // Split nodes using current percentages
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Left, m_leftDockPercent, &m_leftDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Right, m_rightDockPercent, &m_rightDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Up, m_topDockPercent, &m_topDock, &m_mainDock);
      ImGui::DockBuilderSplitNode(m_mainDock, ImGuiDir_Down, m_bottomDockPercent, &m_bottomDock, &m_mainDock);

      m_centerDock = m_mainDock;

      ImGui::DockBuilderFinish(dockSpaceID);
      m_dockNeedsUpdate = false;
    }

    ImGui::DockSpaceOverViewport(dockSpaceID, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    // The frame's dock layout, including any splitter drag, is final here, and docked windows read their node's rect when
    // they begin later in the frame. Tab bars, the splitter and the dockspace background were already drawn from the
    // unclamped layout, so while a splitter is held past a floor those follow the mouse.
    enforceDockedWindowMinimumSizes(dockSpaceID);
  }

  void ImGuiInstance::enforceDockedWindowMinimumSizes(const ImGuiID dockSpaceID) const
  {
    if (m_dockedWindowMinimumSizes.empty())
    {
      return;
    }

    ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockSpaceID);
    if (root == nullptr || root->Size.x <= 0.0f || root->Size.y <= 0.0f)
    {
      return;
    }

    enforceDockNodeMinimumSize(root);
  }

  void ImGuiInstance::enforceDockNodeMinimumSize(ImGuiDockNode* node) const
  {
    if (node->IsLeafNode())
    {
      return;
    }

    ImGuiDockNode* child0 = node->ChildNodes[0];
    ImGuiDockNode* child1 = node->ChildNodes[1];

    if (child0->IsVisible && child1->IsVisible)
    {
      const int axis = node->SplitAxis;
      const float spacing = ImGui::GetStyle().DockingSeparatorSize;
      const float available = std::max(node->Size[axis] - spacing, 0.0f);
      const float minimum0 = getDockNodeMinimumSize(child0)[axis];
      const float minimum1 = getDockNodeMinimumSize(child1)[axis];

      // When the floors cannot all fit, ImGui's own split is left alone rather than traded for a different violation.
      if (minimum0 + minimum1 <= available && (child0->Size[axis] < minimum0 || child1->Size[axis] < minimum1))
      {
        // Only this split moves. A splitter drag may have locked the size of nodes deeper in either child, and ImGui
        // lays this split out from SizeRef on later frames, so SizeRef must hold the clamped result.
        const float size0 = std::clamp(std::floor(child0->Size[axis]), minimum0, available - minimum1);
        const float size1 = available - size0;

        child0->SizeRef[axis] = size0;
        child1->SizeRef[axis] = size1;

        ImVec2 size0Vec = node->Size;
        ImVec2 size1Vec = node->Size;
        size0Vec[axis] = size0;
        size1Vec[axis] = size1;

        ImVec2 pos1 = node->Pos;
        pos1[axis] += size0 + spacing;

        layoutDockNodeSubtree(child0, node->Pos, size0Vec);
        layoutDockNodeSubtree(child1, pos1, size1Vec);
      }
    }

    if (child0->IsVisible)
    {
      enforceDockNodeMinimumSize(child0);
    }

    if (child1->IsVisible)
    {
      enforceDockNodeMinimumSize(child1);
    }
  }

  // Mirrors the size policy of ImGui's DockNodeTreeUpdatePosSize, which is not exported, so nodes below a clamped split
  // keep the sizes ImGui gives them, including sizes a splitter drag just locked. It never writes SizeRef.
  void ImGuiInstance::layoutDockNodeSubtree(ImGuiDockNode* node,
                                            const ImVec2 pos,
                                            const ImVec2 size)
  {
    node->Pos = pos;
    node->Size = size;

    if (node->IsLeafNode())
    {
      return;
    }

    ImGuiDockNode* child0 = node->ChildNodes[0];
    ImGuiDockNode* child1 = node->ChildNodes[1];

    ImVec2 size0 = size;
    ImVec2 size1 = size;
    ImVec2 pos1 = pos;

    if (child0->IsVisible && child1->IsVisible)
    {
      const ImGuiStyle& style = ImGui::GetStyle();
      const int axis = node->SplitAxis;
      const float available = std::max(size[axis] - style.DockingSeparatorSize, 0.0f);
      const float minimumEach = std::trunc(std::min(available, style.WindowMinSize[axis] * 2.0f) * 0.5f);

      if (child0->SizeRef[axis] != 0.0f && child1->HasCentralNodeChild)
      {
        size0[axis] = std::min(available - minimumEach, child0->SizeRef[axis]);
        size1[axis] = available - size0[axis];
      }
      else if (child1->SizeRef[axis] != 0.0f && child0->HasCentralNodeChild)
      {
        size1[axis] = std::min(available - minimumEach, child1->SizeRef[axis]);
        size0[axis] = available - size1[axis];
      }
      else
      {
        const float totalRef = child0->SizeRef[axis] + child1->SizeRef[axis];
        const float ratio = totalRef > 0.0f ? child0->SizeRef[axis] / totalRef : 0.5f;
        size0[axis] = std::max(minimumEach, std::trunc(available * ratio + 0.5f));
        size1[axis] = available - size0[axis];
      }

      pos1[axis] += style.DockingSeparatorSize + size0[axis];
    }

    if (child0->IsVisible)
    {
      layoutDockNodeSubtree(child0, pos, size0);
    }

    if (child1->IsVisible)
    {
      layoutDockNodeSubtree(child1, pos1, size1);
    }
  }

  ImVec2 ImGuiInstance::getDockNodeMinimumSize(const ImGuiDockNode* node) const
  {
    const ImGuiStyle& style = ImGui::GetStyle();

    if (node->IsLeafNode())
    {
      ImVec2 minimumSize = style.WindowMinSize;
      const float contentScale = m_window->getContentScale();

      for (const ImGuiWindow* window : node->Windows)
      {
        if (const auto entry = m_dockedWindowMinimumSizes.find(window->ID); entry != m_dockedWindowMinimumSizes.end())
        {
          // Whole pixels, because dock layout and window rects are truncated to them.
          minimumSize.x = std::max(minimumSize.x, std::ceil(entry->second.x * contentScale));
          minimumSize.y = std::max(minimumSize.y, std::ceil(entry->second.y * contentScale));
        }
      }

      return minimumSize;
    }

    const ImGuiDockNode* child0 = node->ChildNodes[0];
    const ImGuiDockNode* child1 = node->ChildNodes[1];

    if (!child0->IsVisible && !child1->IsVisible)
    {
      return style.WindowMinSize;
    }

    if (!child0->IsVisible || !child1->IsVisible)
    {
      return getDockNodeMinimumSize(child0->IsVisible ? child0 : child1);
    }

    const ImVec2 minimum0 = getDockNodeMinimumSize(child0);
    const ImVec2 minimum1 = getDockNodeMinimumSize(child1);
    const int axis = node->SplitAxis;

    ImVec2 minimumSize;
    minimumSize[axis] = minimum0[axis] + style.DockingSeparatorSize + minimum1[axis];
    minimumSize[axis ^ 1] = std::max(minimum0[axis ^ 1], minimum1[axis ^ 1]);

    return minimumSize;
  }

  float ImGuiInstance::getViewportDpiScale(ImGuiViewport* viewport)
  {
    // ImGui would otherwise take the DPI of the monitor holding most of the window. Rescaling a window for a new DPI
    // resizes it around its top-left corner, which can hand that majority back to the previous monitor, so a window
    // straddling the border changed DPI every frame. The DPI only changes once another monitor holds more than
    // ratio / (1 + ratio) of the window, where ratio is the size change the switch causes; after that resize the
    // previous monitor can't hold as much. The margin keeps rounding from reopening the loop.
    const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    const auto overlapArea = [viewport](const ImGuiPlatformMonitor& monitor) {
      const float width = std::min(viewport->Pos.x + viewport->Size.x, monitor.MainPos.x + monitor.MainSize.x) -
                          std::max(viewport->Pos.x, monitor.MainPos.x);
      const float height = std::min(viewport->Pos.y + viewport->Size.y, monitor.MainPos.y + monitor.MainSize.y) -
                           std::max(viewport->Pos.y, monitor.MainPos.y);
      return std::max(width, 0.0f) * std::max(height, 0.0f);
    };

    float totalArea = 0.0f;
    float bestScale = 0.0f;
    float bestArea = 0.0f;
    for (const ImGuiPlatformMonitor& monitor : platformIO.Monitors)
    {
      if (monitor.DpiScale <= 0.0f)
      {
        continue;
      }

      totalArea += overlapArea(monitor);

      // Monitors sharing a scale count together, so a window spanning two of them isn't split between them.
      float scaleArea = 0.0f;
      for (const ImGuiPlatformMonitor& other : platformIO.Monitors)
      {
        if (other.DpiScale == monitor.DpiScale)
        {
          scaleArea += overlapArea(other);
        }
      }

      if (scaleArea > bestArea)
      {
        bestArea = scaleArea;
        bestScale = monitor.DpiScale;
      }
    }

    const float currentScale = viewport->DpiScale;

    if (bestArea <= 0.0f)
    {
      return currentScale > 0.0f ? currentScale : 1.0f;
    }

    if (currentScale <= 0.0f || bestScale == currentScale)
    {
      return bestScale;
    }

    const float ratio = std::max(bestScale, currentScale) / std::min(bestScale, currentScale);
    const float switchShare = ratio / (1.0f + ratio) + 0.05f;

    return bestArea / totalArea > switchShare ? bestScale : currentScale;
  }

  void ImGuiInstance::renderPlatformWindows()
  {
    if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
      return;
    }

    // Creates, resizes and destroys the OS windows. The backend waits for the device to be idle before destroying or
    // rebuilding a window's swapchain.
    ImGui::UpdatePlatformWindows();

    // Skips minimized windows, whose swapchains would be zero-sized.
    ImGui::RenderPlatformWindowsDefault();
  }

  void ImGuiInstance::renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    ImGui_ImplVulkan_RenderDrawData(
      ImGui::GetDrawData(),
      static_cast<VkCommandBuffer>(*commandBuffer->m_commandBuffers[commandBuffer->m_currentFrame]),
      nullptr
    );
  }

} // namespace vke