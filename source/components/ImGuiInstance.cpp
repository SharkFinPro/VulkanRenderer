#include "ImGuiInstance.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "../utilities/Buffers.h"

#include "../components/Window.h"
#include "../components/Instance.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include "../pipeline/RenderPass.h"
#include "../pipeline/GuiPipeline.h"

ImGuiInstance::ImGuiInstance(VkCommandPool& commandPool, const std::shared_ptr<Window>& window,
                             const std::unique_ptr<Instance>& instance,
                             const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::unique_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const std::unique_ptr<GuiPipeline>& guiPipeline)
{
  ImGui::CreateContext();

  window->initImGui();

  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = instance->getInstance();
  init_info.PhysicalDevice = physicalDevice->getPhysicalDevice();
  init_info.Device = logicalDevice->getDevice();
  init_info.Queue = logicalDevice->getGraphicsQueue();
  init_info.DescriptorPool = guiPipeline->getPool();
  init_info.RenderPass = renderPass->getRenderPass();
  init_info.MSAASamples = physicalDevice->getMsaaSamples();

  SwapChainSupportDetails swapChainSupport = physicalDevice->getSwapChainSupport();

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }
  init_info.MinImageCount = imageCount;
  init_info.ImageCount = imageCount;

  ImGui_ImplVulkan_Init(&init_info);

  VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice->getDevice(), commandPool);
  ImGui_ImplVulkan_CreateFontsTexture();
  Buffers::endSingleTimeCommands(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

ImGuiInstance::~ImGuiInstance()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}