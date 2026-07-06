#include "RenderingManager.h"
#include "FrameScheduler.h"
#include "ImageResource.h"
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
                                     std::string sceneViewName,
                                     const bool useDockspace,
                                     const bool rayTracingEnabled,
                                     const std::shared_ptr<AssetManager>& assetManager)
    : m_logicalDevice(std::move(logicalDevice)),
      m_surface(std::move(surface)),
      m_window(std::move(window)),
      m_useDockspace(useDockspace),
      m_sceneViewName(std::move(sceneViewName)),
      m_renderer2D(std::make_shared<Renderer2D>(assetManager)),
      m_rayTracingEnabled(rayTracingEnabled && m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
  {
    createCommandPool();

    m_frameScheduler = std::make_shared<FrameScheduler>(m_logicalDevice);

    m_renderer3D = std::make_shared<Renderer3D>(m_logicalDevice, assetManager, m_window);

    m_offscreenCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
    m_swapchainCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

    m_swapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window, m_surface, m_commandPool);
    m_frameScheduler->updateRenderFinishedSemaphores(static_cast<uint32_t>(m_swapChain->getImages().size()));

    m_renderTarget = std::make_shared<RenderTarget>(m_logicalDevice, m_commandPool);

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
    uint32_t imageIndex;
    auto result = m_frameScheduler->acquireNextImage(m_swapChain->getSwapChain(), &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
      m_framebufferResized = false;
      recreateSwapChain();

      // The frame is abandoned with its compute work already submitted; bring the timeline up
      // to the frame's final value (recreateSwapChain left the device idle).
      m_frameScheduler->completeAbortedFrame();
      return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_renderer3D->updateLightingManager(lightingManager, currentFrame);

    renderGuiScene(currentFrame);

    recordOffscreenCommandBuffer(pipelineManager, lightingManager, currentFrame);

    recordSwapchainCommandBuffer(pipelineManager, currentFrame, imageIndex);

    result = m_frameScheduler->queuePresent(m_swapChain->getSwapChain(), imageIndex);

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

  bool RenderingManager::isSceneFocused() const
  {
    return m_sceneIsFocused;
  }

  vk::DescriptorSetLayout RenderingManager::getOffscreenImageDescriptorSetLayout() const
  {
    return m_renderTarget->getOffscreenImageDescriptorSetLayout();
  }

  vk::Format RenderingManager::getSwapChainImageFormat() const
  {
    return m_swapChain->getImageFormat();
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

    m_logicalDevice->getPhysicalDevice()->updateSwapChainSupportDetails();

    auto newSwapChain = std::make_shared<SwapChain>(m_logicalDevice, m_window, m_surface, m_commandPool,
                                                    m_swapChain->getSwapChain());
    m_swapChain = std::move(newSwapChain);

    m_frameScheduler->updateRenderFinishedSemaphores(static_cast<uint32_t>(m_swapChain->getImages().size()));

    if (m_offscreenViewportExtent.width == 0 || m_offscreenViewportExtent.height == 0)
    {
      return;
    }

    m_renderTarget->recreateImageResources(m_offscreenViewportExtent);
    m_renderer3D->getMousePicker()->setViewportExtent(m_offscreenViewportExtent);
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

  std::shared_ptr<FrameScheduler> RenderingManager::getFrameScheduler() const
  {
    return m_frameScheduler;
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
    if (!m_useDockspace)
    {
      const auto currentOffscreenViewportExtent = m_swapChain->getExtent();

      if (m_offscreenViewportExtent.width != currentOffscreenViewportExtent.width ||
          m_offscreenViewportExtent.height != currentOffscreenViewportExtent.height)
      {
        m_offscreenViewportExtent = currentOffscreenViewportExtent;

        m_logicalDevice->waitIdle();

        m_renderTarget->recreateImageResources(m_offscreenViewportExtent);
        m_renderer3D->getMousePicker()->setViewportExtent(m_offscreenViewportExtent);
      }

      m_sceneIsFocused = !ImGui::GetIO().WantCaptureMouse;
      m_renderer3D->getMousePicker()->setViewportPos({ 0.0f, 0.0f });

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

      m_renderTarget->recreateImageResources(m_offscreenViewportExtent);
      m_renderer3D->getMousePicker()->setViewportExtent(m_offscreenViewportExtent);
    }

    m_renderer3D->getMousePicker()->setViewportPos(ImGui::GetCursorScreenPos());

    const auto offscreenImageDescriptorSet = m_renderTarget->getOffscreenResolveImageResource(currentFrame).getDescriptorSet();

    ImGui::Image(static_cast<ImTextureRef>(offscreenImageDescriptorSet), contentRegionAvailable);

    ImGui::End();
  }

  void RenderingManager::recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      const std::shared_ptr<LightingManager>& lightingManager,
                                                      const uint32_t currentFrame) const
  {
    auto renderShadowMaps = [this, currentFrame, lightingManager, pipelineManager] {
      if (m_rayTracingEnabled)
      {
        return;
      }

      m_renderer3D->renderShadowMaps(lightingManager, m_offscreenCommandBuffer, pipelineManager, currentFrame);
    };

    auto recordMousePicking = [this, currentFrame, pipelineManager](const RenderInfo& renderInfo) {
      m_renderTarget->beginMousePickingRendering(renderInfo.commandBuffer, currentFrame);

      m_renderer3D->renderMousePicking(&renderInfo, pipelineManager);

      renderInfo.commandBuffer->endRendering();
    };

    auto recordOffscreenRendering = [this, currentFrame, lightingManager, pipelineManager](const RenderInfo& renderInfo) {
      if (m_rayTracingEnabled)
      {
        m_renderTarget->beginRayTracingRendering(renderInfo.commandBuffer, currentFrame);
        m_renderer3D->doRayTracing(&renderInfo, pipelineManager, lightingManager, m_renderTarget->getOffscreenRayTracingImageResource(currentFrame));
        m_renderTarget->endRayTracingRendering(renderInfo.commandBuffer, currentFrame);

        return;
      }

      m_renderTarget->beginOffscreenRendering(renderInfo.commandBuffer, currentFrame);

      m_renderer3D->render(&renderInfo, pipelineManager, lightingManager);

      constexpr vk::ClearAttachment clearAttachment{
        .aspectMask = vk::ImageAspectFlagBits::eDepth,
        .clearValue = vk::ClearValue{ {1.0f, 0} }
      };

      const vk::ClearRect clearRect{
        .rect = {
          .offset = { 0, 0 },
          .extent = renderInfo.extent
        },
        .baseArrayLayer = 0,
        .layerCount = 1
      };

      renderInfo.commandBuffer->clearAttachments({ clearAttachment }, { clearRect });

      RenderInfo renderInfo2D = renderInfo;
      renderInfo2D.extent = vk::Extent2D{
        .width = static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.width) / m_window->getContentScale()),
        .height = static_cast<uint32_t>(static_cast<float>(m_offscreenViewportExtent.height) / m_window->getContentScale()),
      };

      m_renderer2D->render(&renderInfo2D, pipelineManager);

      m_renderTarget->endOffscreenRendering(renderInfo2D.commandBuffer, currentFrame);
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
          renderInfo.extent.height == 0)
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

    m_frameScheduler->submitOffscreenCommandBuffer(m_offscreenCommandBuffer->getCommandBuffer());

    if (m_offscreenViewportExtent.width != 0 &&
        m_offscreenViewportExtent.height != 0)
    {
      m_frameScheduler->waitForOffscreenWork();
      m_renderer3D->handleRenderedMousePickingImage(m_renderTarget->getMousePickingColorImageResource(currentFrame).getImage());
    }
  }

  void RenderingManager::recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                      uint32_t currentFrame,
                                                      const uint32_t imageIndex) const
  {
    m_swapchainCommandBuffer->setCurrentFrame(currentFrame);

    m_swapchainCommandBuffer->resetCommandBuffer();

    m_swapchainCommandBuffer->record([this, pipelineManager, currentFrame, imageIndex]
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

      m_swapChain->beginRendering(imageIndex, renderInfo.commandBuffer);

      if (!m_useDockspace &&
          m_offscreenViewportExtent.width != 0 &&
          m_offscreenViewportExtent.height != 0)
      {
        pipelineManager->bindGraphicsPipeline(renderInfo.commandBuffer, PipelineType::offscreenToSwapchain);

        pipelineManager->bindGraphicsPipelineDescriptorSet(
          renderInfo.commandBuffer,
          PipelineType::offscreenToSwapchain,
          m_renderTarget->getOffscreenImageDescriptorSet(currentFrame),
          0
        );

        renderInfo.commandBuffer->draw(4, 1, 0, 0);
      }

      ImGuiInstance::render(renderInfo.commandBuffer);

      m_swapChain->endRendering(imageIndex, renderInfo.commandBuffer);
    });

    m_frameScheduler->submitSwapchainCommandBuffer(imageIndex, m_swapchainCommandBuffer->getCommandBuffer());
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