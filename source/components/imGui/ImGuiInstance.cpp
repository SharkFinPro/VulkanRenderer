#include "ImGuiInstance.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../instance/Instance.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../renderingManager/legacyRenderer/RenderPass.h"
#include "../window/Window.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui_internal.h>

namespace vke {

  ImGuiInstance::ImGuiInstance(const std::shared_ptr<Window>& window,
                               const std::shared_ptr<Instance>& instance,
                               std::shared_ptr<LogicalDevice> logicalDevice,
                               const std::shared_ptr<RenderPass>& renderPass,
                               const EngineConfig::ImGui& config)
    : m_logicalDevice(std::move(logicalDevice)), m_useDockSpace(config.useDockspace)
  {
    createDescriptorPool(config.maxTextures);

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(window->getWindow(), true);

    initFromWindow(window);

    const SwapChainSupportDetails swapChainSupport = m_logicalDevice->getPhysicalDevice()->getSwapChainSupport();

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    ImGui_ImplVulkan_InitInfo initInfo {
      .Instance = instance->m_instance,
      .PhysicalDevice = m_logicalDevice->getPhysicalDevice()->m_physicalDevice,
      .Device = m_logicalDevice->m_device,
      .Queue = m_logicalDevice->getGraphicsQueue(),
      .DescriptorPool = descriptorPool,
      .MinImageCount = imageCount,
      .ImageCount = imageCount,
      .PipelineInfoMain {
        .RenderPass = renderPass ? renderPass->getRenderPass() : nullptr,
        .MSAASamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples()
      }
    };

    if (renderPass == nullptr)
    {
      initInfo.UseDynamicRendering = true;

      constexpr VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

      initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat()
      };
    }

    ImGui_ImplVulkan_Init(&initInfo);

    if (m_useDockSpace)
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

    m_logicalDevice->destroyDescriptorPool(descriptorPool);
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

  void ImGuiInstance::renderDrawData(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    ImGui_ImplVulkan_RenderDrawData(
      ImGui::GetDrawData(),
      commandBuffer->m_commandBuffers[commandBuffer->m_currentFrame],
      nullptr
    );
  }

  ImGuiContext* ImGuiInstance::getImGuiContext()
  {
    return ImGui::GetCurrentContext();
  }

  void ImGuiInstance::createDescriptorPool(const uint32_t maxImGuiTextures)
  {
    const std::array<VkDescriptorPoolSize, 11> poolSizes {
      {
        {VK_DESCRIPTOR_TYPE_SAMPLER, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_logicalDevice->getMaxFramesInFlight() * maxImGuiTextures},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_logicalDevice->getMaxFramesInFlight()},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_logicalDevice->getMaxFramesInFlight()}
      }};

    const VkDescriptorPoolCreateInfo poolCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * maxImGuiTextures,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
  }

  void ImGuiInstance::markDockNeedsUpdate()
  {
    m_dockNeedsUpdate = true;
  }

  void ImGuiInstance::initFromWindow(const std::shared_ptr<Window>& window)
  {
    float xscale, yscale;
    glfwGetWindowContentScale(window->getWindow(), &xscale, &yscale);

    ImGui::GetStyle().ScaleAllSizes(xscale);
    ImGui::GetIO().FontGlobalScale = xscale;

    m_contentScale = xscale;

    window->on<ContentScaleEvent>([this](const ContentScaleEvent& e) {
      ImGui::GetStyle().ScaleAllSizes(1.0f / m_contentScale);
      ImGui::GetStyle().ScaleAllSizes(e.xscale);
      ImGui::GetIO().FontGlobalScale = e.xscale;

      m_contentScale = e.xscale;

      markDockNeedsUpdate();
    });
  }
} // namespace vke