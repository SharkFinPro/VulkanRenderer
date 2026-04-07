#include "RenderingManager.h"
#include "ImageResource.h"
#include "Renderer.h"
#include "RenderTarget.h"
#include "renderer2D/Renderer2D.h"
#include "renderer3D/Renderer3D.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../imGui/ImGuiInstance.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../lighting/LightingManager.h"
#include "../window/SwapChain.h"
#include "../window/Window.h"
#include "renderer3D/MousePicker.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include <stdexcept>

namespace vke {

  RenderingManager::RenderingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                     std::shared_ptr<Surface> surface,
                                     std::shared_ptr<Window> window,
                                     const bool shouldRenderOffscreen,
                                     std::string sceneViewName,
                                     const std::shared_ptr<AssetManager>& assetManager)
    : m_logicalDevice(std::move(logicalDevice)),
      m_surface(std::move(surface)),
      m_window(std::move(window)),
      m_shouldRenderOffscreen(shouldRenderOffscreen),
      m_sceneViewName(std::move(sceneViewName)),
      m_renderer2D(std::make_shared<Renderer2D>(assetManager))
  {
    createCommandPool();

    m_renderer3D = std::make_shared<Renderer3D>(m_logicalDevice, assetManager, m_window);

    m_offscreenCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
    m_swapchainCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

    m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window, m_surface);

    m_renderer = std::make_shared<Renderer>(m_logicalDevice, m_swapChain, m_commandPool);

    m_framebufferResizeEventListener = m_window->on<FramebufferResizeEvent>([this]([[maybe_unused]] const FramebufferResizeEvent& e) {
      m_framebufferResized = true;
    });
  }

  RenderingManager::~RenderingManager()
  {
    m_window->removeListener(m_framebufferResizeEventListener);
  }

  void RenderingManager::doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                                     const std::shared_ptr<LightingManager>& lightingManager,
                                     const uint32_t currentFrame)
  {
    m_logicalDevice->waitForGraphicsFences(currentFrame);

    uint32_t imageIndex;
    auto result = m_logicalDevice->acquireNextImage(currentFrame, m_swapChain->getSwapChain(), &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
      m_framebufferResized = false;
      recreateSwapChain();
      return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_renderer3D->updateLightingManager(lightingManager, currentFrame);

    renderGuiScene(currentFrame);

    m_logicalDevice->resetGraphicsFences(currentFrame);

    recordOffscreenCommandBuffer(pipelineManager, lightingManager, currentFrame);

    recordSwapchainCommandBuffer(pipelineManager, lightingManager, currentFrame, imageIndex);

    result = m_logicalDevice->queuePresent(currentFrame, m_swapChain->getSwapChain(), &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized)
    {
      m_framebufferResized = false;
      recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
      throw std::runtime_error("failed to present swap chain image!");
    }
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

      m_renderer->resetMousePickingImageResources(m_offscreenViewportExtent);
      m_renderer3D->getMousePicker()->setViewportExtent(m_offscreenViewportExtent);
    }
    else
    {
      m_renderer->resetMousePickingImageResources(m_swapChain->getExtent());
      m_renderer3D->getMousePicker()->setViewportExtent(m_swapChain->getExtent());
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

  bool RenderingManager::supportsRayTracing() const
  {
    return m_logicalDevice->getPhysicalDevice()->supportsRayTracing();
  }

  void RenderingManager::enableRayTracing()
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    m_rayTracingEnabled = true;
  }

  void RenderingManager::disableRayTracing()
  {
    m_rayTracingEnabled = false;
  }

  bool RenderingManager::isRayTracingEnabled() const
  {
    return m_rayTracingEnabled;
  }

  void RenderingManager::renderGuiScene(const uint32_t currentFrame)
  {
    if (!m_shouldRenderOffscreen)
    {
      return;
    }

    ImGui::Begin(m_sceneViewName.c_str());

    m_sceneIsFocused = ImGui::IsWindowFocused();

    const auto contentRegionAvailable = ImGui::GetContentRegionAvail();

    const vk::Extent2D currentOffscreenViewportExtent {
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

      m_renderer->resetMousePickingImageResources(m_offscreenViewportExtent);
      m_renderer3D->getMousePicker()->setViewportExtent(m_offscreenViewportExtent);
    }

    m_offscreenViewportPos = ImGui::GetCursorScreenPos();
    m_renderer3D->getMousePicker()->setViewportPos(m_offscreenViewportPos);

    ImGui::Image(static_cast<ImTextureRef>(m_renderer->getOffscreenImageDescriptorSet(currentFrame)), contentRegionAvailable);

    ImGui::End();
  }

  void RenderingManager::recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      const std::shared_ptr<LightingManager>& lightingManager,
                                                      const uint32_t currentFrame) const
  {
    auto renderShadowMaps = [&] {
      if (m_rayTracingEnabled)
      {
        return;
      }

      m_renderer3D->renderShadowMaps(lightingManager, m_offscreenCommandBuffer, pipelineManager, currentFrame);
    };

    auto recordMousePicking = [&](const RenderInfo& renderInfo) {
      m_renderer->beginMousePickingRendering(currentFrame, renderInfo.extent, renderInfo.commandBuffer);

      m_renderer3D->renderMousePicking(&renderInfo, pipelineManager);

      renderInfo.commandBuffer->endRendering();
    };

    auto recordOffscreenRendering = [&](const RenderInfo& renderInfo) {
      if (m_rayTracingEnabled)
      {
        m_renderer->beginRayTracingRendering(m_offscreenCommandBuffer, currentFrame);
        m_renderer3D->doRayTracing(&renderInfo, pipelineManager, lightingManager, m_renderer->getRayTracingImageResource(currentFrame));
        m_renderer->endRayTracingRendering(m_offscreenCommandBuffer, currentFrame);

        return;
      }

      m_renderer->beginOffscreenRendering(currentFrame, m_offscreenViewportExtent, m_offscreenCommandBuffer);

      m_renderer3D->render(&renderInfo, pipelineManager, lightingManager);

      resetDepthBuffer(m_offscreenCommandBuffer, m_offscreenViewportExtent);

      RenderInfo renderInfo2D = renderInfo;
      renderInfo2D.extent = vk::Extent2D{
        .width = static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.width) / m_window->getContentScale()),
        .height = static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.height) / m_window->getContentScale()),
      };

      m_renderer2D->render(&renderInfo2D, pipelineManager);

      renderInfo2D.commandBuffer->endRendering();
    };

    m_offscreenCommandBuffer->setCurrentFrame(currentFrame);

    m_offscreenCommandBuffer->resetCommandBuffer();

    m_offscreenCommandBuffer->record([this, currentFrame, renderShadowMaps, recordMousePicking, recordOffscreenRendering]
    {
      const RenderInfo renderInfo {
        .commandBuffer = m_offscreenCommandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = {},
        .viewMatrix = {},
        .extent = m_offscreenViewportExtent
      };

      if (renderInfo.extent.width == 0 ||
          renderInfo.extent.height == 0 ||
          !m_shouldRenderOffscreen)
      {
        return;
      }

      renderShadowMaps();

      const vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(renderInfo.extent.width),
        .height = static_cast<float>(renderInfo.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      renderInfo.commandBuffer->setViewport(viewport);

      const vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = renderInfo.extent
      };
      renderInfo.commandBuffer->setScissor(scissor);

      recordMousePicking(renderInfo);

      recordOffscreenRendering(renderInfo);
    });

    m_logicalDevice->submitOffscreenCommandBuffer(currentFrame, m_offscreenCommandBuffer->getCommandBuffer());

    if (m_shouldRenderOffscreen)
    {
      m_logicalDevice->waitForOffscreenFence(currentFrame);
      m_renderer3D->handleRenderedMousePickingImage(m_renderer->getMousePickingRenderTarget()->getColorImageResource(0).getImage());
    }
  }

  void RenderingManager::recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      const std::shared_ptr<LightingManager>& lightingManager,
                                                      uint32_t currentFrame,
                                                      const uint32_t imageIndex) const
  {
    m_swapchainCommandBuffer->setCurrentFrame(currentFrame);

    m_swapchainCommandBuffer->resetCommandBuffer();

    m_swapchainCommandBuffer->record([this, pipelineManager, lightingManager, currentFrame, imageIndex]
    {
      const RenderInfo renderInfo {
        .commandBuffer = m_swapchainCommandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = {},
        .viewMatrix = {},
        .extent = m_swapChain->getExtent()
      };

      const vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(renderInfo.extent.width),
        .height = static_cast<float>(renderInfo.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      renderInfo.commandBuffer->setViewport(viewport);

      const vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = renderInfo.extent
      };
      renderInfo.commandBuffer->setScissor(scissor);

      m_renderer->beginSwapchainRendering(imageIndex, m_swapChain->getExtent(), renderInfo.commandBuffer, m_swapChain);

      if (!m_shouldRenderOffscreen)
      {
        m_renderer3D->render(&renderInfo, pipelineManager, lightingManager);

        resetDepthBuffer(m_swapchainCommandBuffer, m_swapChain->getExtent());

        RenderInfo renderInfo2D = renderInfo;
        renderInfo2D.extent = vk::Extent2D{
          .width = static_cast<uint32_t>(static_cast<float>(renderInfo.extent.width) / m_window->getContentScale()),
          .height = static_cast<uint32_t>(static_cast<float>(renderInfo.extent.height) / m_window->getContentScale()),
        };

        m_renderer2D->render(&renderInfo2D, pipelineManager);
      }

      ImGuiInstance::render(renderInfo.commandBuffer);

      m_renderer->endSwapchainRendering(imageIndex, renderInfo.commandBuffer, m_swapChain);
    });

    m_logicalDevice->submitSwapchainCommandBuffer(currentFrame, m_swapchainCommandBuffer->getCommandBuffer());
  }

  void RenderingManager::resetDepthBuffer(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          const vk::Extent2D extent)
  {
    constexpr vk::ClearAttachment clearAttachment{
      .aspectMask = vk::ImageAspectFlagBits::eDepth,
      .clearValue = vk::ClearValue{ {1.0f, 0} }
    };

    const vk::ClearRect clearRect{
      .rect = {
        .offset = { 0, 0 },
        .extent = extent
      },
      .baseArrayLayer = 0,
      .layerCount = 1
    };

    commandBuffer->clearAttachments({ clearAttachment }, { clearRect });
  }

  void RenderingManager::createCommandPool()
  {
    const vk::CommandPoolCreateInfo poolInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }
} // namespace vke