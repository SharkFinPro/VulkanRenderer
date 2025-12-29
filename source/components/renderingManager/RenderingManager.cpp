#include "RenderingManager.h"
#include "dynamicRenderer/DynamicRenderer.h"
#include "legacyRenderer/LegacyRenderer.h"
#include "Renderer.h"
#include "renderer2D/Renderer2D.h"
#include "renderer3D/Renderer3D.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../lighting/LightingManager.h"
#include "../window/SwapChain.h"
#include "../window/Window.h"
#include "../mousePicker/MousePicker.h"
#include "../pipelines/implementations/GuiPipeline.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdexcept>

namespace vke {

  RenderingManager::RenderingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                     std::shared_ptr<Surface> surface,
                                     std::shared_ptr<Window> window,
                                     VkCommandPool commandPool,
                                     const bool shouldRenderOffscreen,
                                     const char* sceneViewName,
                                     const std::shared_ptr<AssetManager>& assetManager)
    : m_logicalDevice(std::move(logicalDevice)),
      m_surface(std::move(surface)),
      m_window(std::move(window)),
      m_commandPool(commandPool),
      m_shouldRenderOffscreen(shouldRenderOffscreen),
      m_sceneViewName(sceneViewName),
      m_renderer2D(std::make_shared<Renderer2D>(assetManager)),
      m_renderer3D(std::make_shared<Renderer3D>(m_logicalDevice, m_window, assetManager, commandPool))
  {
    m_offscreenCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
    m_swapchainCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

    m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window, m_surface);

    // m_renderer = std::make_shared<LegacyRenderer>(m_logicalDevice, m_swapChain, m_commandPool);
    m_renderer = std::make_shared<DynamicRenderer>(m_logicalDevice, m_swapChain, m_commandPool);

    m_window->on<FramebufferResizeEvent>([this]([[maybe_unused]] const FramebufferResizeEvent& e) {
      m_framebufferResized = true;
    });
  }

  void RenderingManager::doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                                     const std::shared_ptr<LightingManager>& lightingManager,
                                     const uint32_t currentFrame)
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

    m_renderer3D->renderShadowMaps(lightingManager, currentFrame);

    m_renderer3D->doMousePicking(imageIndex, currentFrame);

    m_offscreenCommandBuffer->setCurrentFrame(currentFrame);
    m_offscreenCommandBuffer->resetCommandBuffer();
    recordOffscreenCommandBuffer(pipelineManager, lightingManager, currentFrame, imageIndex);
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
  }

  std::shared_ptr<SwapChain> RenderingManager::getSwapChain() const
  {
    return m_swapChain;
  }

  std::shared_ptr<Renderer> RenderingManager::getRenderer() const
  {
    return m_renderer;
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

    m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window, m_surface);

    m_renderer->resetSwapchainImageResources(m_swapChain);

    if (m_shouldRenderOffscreen)
    {
      if (m_offscreenViewportExtent.width == 0 || m_offscreenViewportExtent.height == 0)
      {
        return;
      }

      m_renderer->resetOffscreenImageResources(m_offscreenViewportExtent);

      m_renderer3D->getMousePicker()->recreateFramebuffer(m_offscreenViewportExtent);
    }
    else
    {
      m_renderer3D->getMousePicker()->recreateFramebuffer(m_swapChain->getExtent());
    }
  }

  void RenderingManager::createNewFrame() const
  {
    m_renderer2D->createNewFrame();

    m_renderer3D->createNewFrame();
  }

  std::shared_ptr<Renderer2D> RenderingManager::getRenderer2D() const
  {
    return m_renderer2D;
  }

  std::shared_ptr<Renderer3D> RenderingManager::getRenderer3D() const
  {
    return m_renderer3D;
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

      m_renderer3D->getMousePicker()->recreateFramebuffer(m_offscreenViewportExtent);
    }

    m_offscreenViewportPos = ImGui::GetCursorScreenPos();
    m_renderer3D->getMousePicker()->setViewportPos(m_offscreenViewportPos);

    ImGui::Image(m_renderer->getOffscreenImageDescriptorSet(imageIndex), contentRegionAvailable);

    ImGui::End();
  }

  void RenderingManager::recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      const std::shared_ptr<LightingManager>& lightingManager,
                                                      uint32_t currentFrame,
                                                      const uint32_t imageIndex) const
  {
    m_offscreenCommandBuffer->record([this, pipelineManager, currentFrame, imageIndex, lightingManager]()
    {
      if (!m_shouldRenderOffscreen ||
          m_offscreenViewportExtent.width == 0 ||
          m_offscreenViewportExtent.height == 0)
      {
        return;
      }

      lightingManager->renderShadowMaps(m_offscreenCommandBuffer, pipelineManager, currentFrame);

      m_renderer->beginOffscreenRendering(imageIndex, m_offscreenViewportExtent, m_offscreenCommandBuffer);

      const RenderInfo renderInfo {
        .commandBuffer = m_offscreenCommandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = {},
        .viewMatrix = {},
        .extent = m_offscreenViewportExtent
      };

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

      m_renderer3D->render(&renderInfo, pipelineManager);

      resetDepthBuffer(*m_offscreenCommandBuffer->getCommandBuffer(), m_offscreenViewportExtent);

      RenderInfo renderInfo2D = renderInfo;
      renderInfo2D.extent = {
        static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.width) / m_window->getContentScale()),
        static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.height) / m_window->getContentScale()),
      };

      m_renderer2D->render(&renderInfo2D, pipelineManager);

      m_renderer->endOffscreenRendering(imageIndex, m_offscreenCommandBuffer);
    });
  }

  void RenderingManager::recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      uint32_t currentFrame,
                                                      const uint32_t imageIndex) const
  {
    m_swapchainCommandBuffer->record([this, pipelineManager, currentFrame, imageIndex]()
    {
      const RenderInfo renderInfo {
        .commandBuffer = m_swapchainCommandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = {},
        .viewMatrix = {},
        .extent = m_swapChain->getExtent()
      };

      m_renderer->beginSwapchainRendering(imageIndex, m_swapChain->getExtent(), renderInfo.commandBuffer, m_swapChain);

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

      if (!m_shouldRenderOffscreen)
      {
        m_renderer3D->render(&renderInfo, pipelineManager);

        resetDepthBuffer(*m_swapchainCommandBuffer->getCommandBuffer(), m_swapChain->getExtent());

        RenderInfo renderInfo2D = renderInfo;
        renderInfo2D.extent = {
          static_cast<uint32_t>(static_cast<float>(renderInfo.extent.width) / m_window->getContentScale()),
          static_cast<uint32_t>(static_cast<float>(renderInfo.extent.height) / m_window->getContentScale()),
        };

        m_renderer2D->render(&renderInfo2D, pipelineManager);
      }

      pipelineManager->getGuiPipeline()->render(&renderInfo);

      m_renderer->endSwapchainRendering(imageIndex, renderInfo.commandBuffer, m_swapChain);
    });
  }

  void RenderingManager::resetDepthBuffer(VkCommandBuffer commandBuffer,
                                          const VkExtent2D extent)
  {
    constexpr VkClearAttachment clearAttachment{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .clearValue = {
        .depthStencil = { 1.0f, 0 }
      }
    };

    const VkClearRect clearRect{
      .rect = {
        .offset = { 0, 0 },
        .extent = extent
      },
      .baseArrayLayer = 0,
      .layerCount = 1
    };

    vkCmdClearAttachments(commandBuffer, 1, &clearAttachment, 1, &clearRect);
  }
} // namespace vke