#include "RenderingManager.h"
#include "DynamicRenderer.h"
#include "LegacyRenderer.h"
#include "Renderer.h"
#include "../PipelineManager.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../core/physicalDevice/PhysicalDevice.h"
#include "../lighting/LightingManager.h"
#include "../window/SwapChain.h"
#include "../MousePicker.h"
#include "../../pipelines/custom/GuiPipeline.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdexcept>

RenderingManager::RenderingManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                   const std::shared_ptr<Window>& window,
                                   const std::shared_ptr<MousePicker>& mousePicker,
                                   VkCommandPool commandPool,
                                   const bool shouldRenderOffscreen,
                                   const char* sceneViewName)
  : m_logicalDevice(logicalDevice), m_window(window),
    m_mousePicker(mousePicker), m_commandPool(commandPool),
    m_shouldRenderOffscreen(shouldRenderOffscreen), m_sceneViewName(sceneViewName)
{
  m_offscreenCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
  m_swapchainCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

  m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window);

  // m_renderer = std::make_shared<LegacyRenderer>(m_logicalDevice, m_swapChain, m_commandPool);
  m_renderer = std::make_shared<DynamicRenderer>(m_logicalDevice, m_swapChain, m_commandPool);
}

void RenderingManager::recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                    uint32_t currentFrame, const uint32_t imageIndex) const
{
  m_offscreenCommandBuffer->record([this, pipelineManager, currentFrame, imageIndex]()
  {
    if (!m_shouldRenderOffscreen ||
        m_offscreenViewportExtent.width == 0 ||
        m_offscreenViewportExtent.height == 0)
    {
      return;
    }

    m_renderer->beginOffscreenRendering(imageIndex, m_offscreenViewportExtent, m_offscreenCommandBuffer);

    pipelineManager->renderGraphicsPipelines(m_offscreenCommandBuffer, m_offscreenViewportExtent,
                                               currentFrame, m_viewPosition, m_viewMatrix);

    m_renderer->endOffscreenRendering(m_offscreenCommandBuffer);
  });
}

void RenderingManager::recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                    uint32_t currentFrame, const uint32_t imageIndex) const
{
  m_swapchainCommandBuffer->record([this, pipelineManager, currentFrame, imageIndex]()
  {
    const RenderInfo renderInfo {
      .commandBuffer = m_swapchainCommandBuffer,
      .currentFrame = currentFrame,
      .viewPosition = m_viewPosition,
      .viewMatrix = m_viewMatrix,
      .extent = m_swapChain->getExtent()
    };

    m_renderer->beginSwapchainRendering(imageIndex, m_swapChain->getExtent(), renderInfo.commandBuffer, m_swapChain);

    if (!m_shouldRenderOffscreen)
    {
      pipelineManager->renderGraphicsPipelines(renderInfo.commandBuffer, m_swapChain->getExtent(),
                                                 currentFrame, m_viewPosition, m_viewMatrix);
    }

    const VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(renderInfo.extent.width),
      .height = static_cast<float>(renderInfo.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    renderInfo.commandBuffer->setViewport(viewport);

    const VkRect2D scissor = {
      .offset = {0, 0},
      .extent = renderInfo.extent
    };
    renderInfo.commandBuffer->setScissor(scissor);

    pipelineManager->getGuiPipeline()->render(&renderInfo);

    m_renderer->endSwapchainRendering(renderInfo.commandBuffer, m_swapChain);
  });
}

void RenderingManager::doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                                   const std::shared_ptr<LightingManager>& lightingManager,
                                   uint32_t& currentFrame)
{
  m_logicalDevice->waitForGraphicsFences(currentFrame);

  uint32_t imageIndex;
  auto result = m_logicalDevice->acquireNextImage(currentFrame, m_swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m_framebufferResized = false;
    recreateSwapChain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  renderGuiScene(imageIndex);

  m_logicalDevice->resetGraphicsFences(currentFrame);

  lightingManager->update(currentFrame, m_viewPosition);

  m_mousePicker->doMousePicking(imageIndex, currentFrame, m_viewPosition, m_viewMatrix,
                                pipelineManager->getRenderObjectsToRender());

  m_offscreenCommandBuffer->setCurrentFrame(currentFrame);
  m_offscreenCommandBuffer->resetCommandBuffer();
  recordOffscreenCommandBuffer(pipelineManager, currentFrame, imageIndex);
  m_logicalDevice->submitOffscreenGraphicsQueue(currentFrame, m_offscreenCommandBuffer->getCommandBuffer());

  m_swapchainCommandBuffer->setCurrentFrame(currentFrame);
  m_swapchainCommandBuffer->resetCommandBuffer();
  recordSwapchainCommandBuffer(pipelineManager, currentFrame, imageIndex);
  m_logicalDevice->submitGraphicsQueue(currentFrame, m_swapchainCommandBuffer->getCommandBuffer());

  result = m_logicalDevice->queuePresent(currentFrame, m_swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
  {
    m_framebufferResized = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % m_logicalDevice->getMaxFramesInFlight();
}

std::shared_ptr<SwapChain> RenderingManager::getSwapChain() const
{
  return m_swapChain;
}

void RenderingManager::setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix)
{
  m_viewPosition = position;
  m_viewMatrix = viewMatrix;
}

std::shared_ptr<Renderer> RenderingManager::getRenderer() const
{
  return m_renderer;
}

void RenderingManager::markFramebufferResized()
{
  m_framebufferResized = true;
}

bool RenderingManager::isSceneFocused() const
{
  return m_sceneIsFocused || !m_shouldRenderOffscreen;
}

void RenderingManager::recreateSwapChain()
{
  int width = 0, height = 0;
  m_window->getFramebufferSize(&width, &height);
  while (width == 0 || height == 0)
  {
    m_window->getFramebufferSize(&width, &height);
    glfwWaitEvents();
  }

  m_logicalDevice->waitIdle();

  m_swapChain.reset();

  m_logicalDevice->getPhysicalDevice()->updateSwapChainSupportDetails();

  m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window);

  m_renderer->resetSwapchainImageResources(m_swapChain);

  if (m_shouldRenderOffscreen)
  {
    if (m_offscreenViewportExtent.width == 0 || m_offscreenViewportExtent.height == 0)
    {
      return;
    }

    m_renderer->resetOffscreenImageResources(m_offscreenViewportExtent);

    m_mousePicker->recreateFramebuffer(m_offscreenViewportExtent);
  }
  else
  {
    m_mousePicker->recreateFramebuffer(m_swapChain->getExtent());
  }
}

void RenderingManager::renderGuiScene(const uint32_t imageIndex)
{
  if (!m_shouldRenderOffscreen)
  {
    return;
  }

  ImGui::Begin(m_sceneViewName);

  m_sceneIsFocused = ImGui::IsWindowFocused();

  const auto contentRegionAvailable = ImGui::GetContentRegionAvail();

  const VkExtent2D currentOffscreenViewportExtent {
    .width = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.x)),
    .height = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.y))
  };

  if (currentOffscreenViewportExtent.width == 0 || currentOffscreenViewportExtent.height == 0)
  {
    m_offscreenViewportExtent = currentOffscreenViewportExtent;
    ImGui::End();
    return;
  }

  if (m_offscreenViewportExtent.width != currentOffscreenViewportExtent.width ||
      m_offscreenViewportExtent.height != currentOffscreenViewportExtent.height)
  {
    m_offscreenViewportExtent = currentOffscreenViewportExtent;

    m_logicalDevice->waitIdle();

    m_renderer->resetOffscreenImageResources(m_offscreenViewportExtent);

    m_mousePicker->recreateFramebuffer(m_offscreenViewportExtent);
  }

  m_offscreenViewportPos = ImGui::GetCursorScreenPos();
  m_mousePicker->setViewportPos(m_offscreenViewportPos);

  ImGui::Image(m_renderer->getOffscreenImageDescriptorSet(imageIndex), contentRegionAvailable);

  ImGui::End();
}
