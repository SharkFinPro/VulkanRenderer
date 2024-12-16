#include "ImGuiInstance.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "../utilities/Buffers.h"

#include "../components/Window.h"
#include "../components/Instance.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include "../pipelines/RenderPass.h"
#include "../pipelines/gui/GuiPipeline.h"

ImGuiInstance::ImGuiInstance(const VkCommandPool& commandPool, const std::shared_ptr<Window>& window,
                             const std::unique_ptr<Instance>& instance,
                             const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const std::unique_ptr<GuiPipeline>& guiPipeline,
                             const bool useDockSpace)
  : useDockSpace(useDockSpace)
{
  ImGui::CreateContext();

  window->initImGui();

  SwapChainSupportDetails swapChainSupport = physicalDevice->getSwapChainSupport();

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

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::GetIO().FontGlobalScale = 1.75f;

  const VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice->getDevice(), commandPool);
  ImGui_ImplVulkan_CreateFontsTexture();
  Buffers::endSingleTimeCommands(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);

  createNewFrame();
}

ImGuiInstance::~ImGuiInstance()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiInstance::createNewFrame() const
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

  ImGui::Begin("WindowDockSpace", nullptr, ImGuiWindowFlags_NoTitleBar);

  ImGui::DockSpace(ImGui::GetID("WindowDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::End();
}
