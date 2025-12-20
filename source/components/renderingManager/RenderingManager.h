#ifndef VKE_RENDERINGMANAGER_H
#define VKE_RENDERINGMANAGER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class AssetManager;
class CommandBuffer;
class LightingManager;
class LogicalDevice;
class MousePicker;
class PipelineManager;
class Renderer;
class Renderer2D;
class SwapChain;
class Window;

class RenderingManager {
public:
  RenderingManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const std::shared_ptr<Window>& window,
                   const std::shared_ptr<MousePicker>& mousePicker,
                   VkCommandPool commandPool,
                   bool shouldRenderOffscreen,
                   const char* sceneViewName,
                   std::shared_ptr<AssetManager> assetManager);

  void doRendering(const std::shared_ptr<PipelineManager>& pipelineManager,
                   const std::shared_ptr<LightingManager>& lightingManager,
                   uint32_t currentFrame);

  [[nodiscard]] std::shared_ptr<SwapChain> getSwapChain() const;

  void setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix);

  [[nodiscard]] std::shared_ptr<Renderer> getRenderer() const;

  void markFramebufferResized();

  [[nodiscard]] bool isSceneFocused() const;

  void recreateSwapChain();

  void enableGrid();

  void disableGrid();

  [[nodiscard]] bool isGridEnabled() const;

  void createNewFrame() const;

  [[nodiscard]] std::shared_ptr<Renderer2D> getRenderer2D() const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<Window> m_window;

  std::shared_ptr<MousePicker> m_mousePicker;

  std::shared_ptr<Renderer> m_renderer;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  std::shared_ptr<CommandBuffer> m_offscreenCommandBuffer;
  std::shared_ptr<CommandBuffer> m_swapchainCommandBuffer;

  std::shared_ptr<SwapChain> m_swapChain;

  glm::vec3 m_viewPosition{};
  glm::mat4 m_viewMatrix{};

  bool m_shouldRenderOffscreen;

  bool m_framebufferResized = false;

  bool m_sceneIsFocused = false;

  VkExtent2D m_offscreenViewportExtent{};

  ImVec2 m_offscreenViewportPos{0, 0};

  const char* m_sceneViewName;

  bool m_shouldRenderGrid = true;

  std::shared_ptr<Renderer2D> m_renderer2D;

  void renderGuiScene(uint32_t imageIndex);

  void recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const std::shared_ptr<LightingManager>& lightingManager,
                                    uint32_t currentFrame,
                                    uint32_t imageIndex) const;

  void recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t currentFrame,
                                    uint32_t imageIndex) const;
};

} // namespace vke

#endif //VKE_RENDERINGMANAGER_H
