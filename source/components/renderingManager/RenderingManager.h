#ifndef VKE_RENDERINGMANAGER_H
#define VKE_RENDERINGMANAGER_H

#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>

namespace vke {

  class AssetManager;
  class CommandBuffer;
  class LightingManager;
  class LogicalDevice;
  class PipelineManager;
  class Renderer;
  class Renderer2D;
  class Renderer3D;
  class Surface;
  class SwapChain;
  class Window;

  class RenderingManager {
  public:
    RenderingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                     std::shared_ptr<Surface> surface,
                     std::shared_ptr<Window> window,
                     bool shouldRenderOffscreen,
                     std::string sceneViewName,
                     const std::shared_ptr<AssetManager>& assetManager);

    ~RenderingManager();

    void doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                     const std::shared_ptr<LightingManager>& lightingManager,
                     uint32_t currentFrame);

    [[nodiscard]] std::shared_ptr<SwapChain> getSwapChain() const;

    [[nodiscard]] std::shared_ptr<Renderer> getRenderer() const;

    [[nodiscard]] bool isSceneFocused() const;

    void recreateSwapChain();

    void createNewFrame() const;

    [[nodiscard]] std::shared_ptr<Renderer2D> getRenderer2D() const;

    [[nodiscard]] std::shared_ptr<Renderer3D> getRenderer3D() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<Surface> m_surface;

    std::shared_ptr<Window> m_window;

    std::shared_ptr<Renderer> m_renderer;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::shared_ptr<CommandBuffer> m_offscreenCommandBuffer;

    std::shared_ptr<CommandBuffer> m_swapchainCommandBuffer;

    std::shared_ptr<CommandBuffer> m_mousePickingCommandBuffer;

    std::shared_ptr<SwapChain> m_swapChain;

    bool m_shouldRenderOffscreen;

    bool m_framebufferResized = false;

    bool m_sceneIsFocused = false;

    VkExtent2D m_offscreenViewportExtent{};

    ImVec2 m_offscreenViewportPos{0, 0};

    std::string m_sceneViewName;

    std::shared_ptr<Renderer2D> m_renderer2D;

    std::shared_ptr<Renderer3D> m_renderer3D;

    void renderGuiScene(uint32_t imageIndex);

    void recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                      const std::shared_ptr<LightingManager>& lightingManager,
                                      uint32_t currentFrame,
                                      uint32_t imageIndex) const;

    void recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                      const std::shared_ptr<LightingManager>& lightingManager,
                                      uint32_t currentFrame,
                                      uint32_t imageIndex) const;

    void recordMousePickingCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                         uint32_t imageIndex,
                                         uint32_t currentFrame) const;

    static void resetDepthBuffer(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                 VkExtent2D extent);

    void doMousePicking(const std::shared_ptr<PipelineManager>& pipelineManager,
                        uint32_t currentFrame,
                        uint32_t imageIndex) const;

    void createCommandPool();
  };

} // namespace vke

#endif //VKE_RENDERINGMANAGER_H
