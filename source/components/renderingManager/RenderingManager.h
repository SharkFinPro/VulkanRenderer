#ifndef VKE_RENDERINGMANAGER_H
#define VKE_RENDERINGMANAGER_H

#include "../../utilities/EventSystem.h"
#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>
#include <string>

struct ImDrawList;

namespace vke {

  class AssetManager;
  class CommandBuffer;
  class FrameScheduler;
  struct FramebufferResizeEvent;
  class LightingManager;
  class LogicalDevice;
  class PipelineManager;
  class RenderTarget;
  class Renderer2D;
  class Renderer3D;
  class Surface;
  class SwapChain;
  class Window;

  // Where the scene image sits, in ImGui screen coordinates (the space ImGui::GetIO().MousePos uses).
  struct SceneViewRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
  };

  using SceneOverlayCallback = std::function<void(ImDrawList* drawList, const SceneViewRect& rect)>;

  class RenderingManager {
  public:
    RenderingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                     std::shared_ptr<Surface> surface,
                     std::shared_ptr<Window> window,
                     std::string sceneViewName,
                     bool useDockspace,
                     bool rayTracingEnabled,
                     const std::shared_ptr<AssetManager>& assetManager);

    ~RenderingManager();

    void doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                     const std::shared_ptr<LightingManager>& lightingManager,
                     uint32_t currentFrame);

    [[nodiscard]] bool isSceneFocused() const;

    [[nodiscard]] bool isSceneHovered() const;

    // True while the right button is held after being pressed over the scene view. The drag keeps it even once the
    // cursor leaves the scene.
    [[nodiscard]] bool isSceneRightDragged() const;

    [[nodiscard]] vk::DescriptorSetLayout getOffscreenImageDescriptorSetLayout() const;

    [[nodiscard]] vk::Format getSwapChainImageFormat() const;

    // Returns false, leaving the swapchain for a later frame, while the window is minimized.
    bool recreateSwapChain();

    void createNewFrame() const;

    [[nodiscard]] std::shared_ptr<Renderer2D> getRenderer2D() const;

    [[nodiscard]] std::shared_ptr<Renderer3D> getRenderer3D() const;

    [[nodiscard]] std::shared_ptr<FrameScheduler> getFrameScheduler() const;

    [[nodiscard]] bool supportsRayTracing() const;

    void enableRayTracing();

    void disableRayTracing();

    [[nodiscard]] bool isRayTracingEnabled() const;

    [[nodiscard]] SceneViewRect getSceneViewRect() const;

    // Called once per frame right after the scene image is drawn; pass nullptr / an empty function to clear.
    void setSceneOverlay(SceneOverlayCallback callback);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<FrameScheduler> m_frameScheduler;

    std::shared_ptr<Surface> m_surface;

    std::shared_ptr<Window> m_window;

    std::shared_ptr<RenderTarget> m_renderTarget;

    vk::raii::CommandPool m_commandPool = nullptr;

    std::shared_ptr<CommandBuffer> m_offscreenCommandBuffer;

    std::shared_ptr<CommandBuffer> m_swapchainCommandBuffer;

    std::shared_ptr<SwapChain> m_swapChain;

    bool m_framebufferResized = false;

    bool m_sceneIsFocused = false;

    bool m_sceneIsHovered = false;

    bool m_sceneOwnsRightDrag = false;

    bool m_useDockspace;

    vk::Extent2D m_offscreenViewportExtent{0, 0};

    std::string m_sceneViewName;

    std::shared_ptr<Renderer2D> m_renderer2D;

    std::shared_ptr<Renderer3D> m_renderer3D;

    EventListener<FramebufferResizeEvent> m_framebufferResizeEventListener;

    bool m_rayTracingEnabled;

    SceneViewRect m_sceneViewRect;

    // Offscreen pixels per unit of the 2D pass's coordinate space, for the window the scene is shown in.
    float m_scenePixelsPer2DUnit = 1.0f;

    SceneOverlayCallback m_sceneOverlay;

    [[nodiscard]] bool isMinimized() const;

    void renderWithoutSwapchain(const std::shared_ptr<PipelineManager>& pipelineManager,
                                const std::shared_ptr<LightingManager>& lightingManager,
                                uint32_t currentFrame);

    // Starts the right-button drag on a press when pressStartsDrag, and ends it on release. Returns whether a drag
    // started this frame.
    bool updateSceneRightDrag(bool pressStartsDrag);

    void renderGuiScene(uint32_t currentFrame);

    void recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                      const std::shared_ptr<LightingManager>& lightingManager,
                                      uint32_t currentFrame) const;

    void recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                      uint32_t currentFrame,
                                      uint32_t imageIndex) const;

    void createCommandPool();
  };

} // namespace vke

#endif //VKE_RENDERINGMANAGER_H
