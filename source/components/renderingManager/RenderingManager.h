#ifndef RENDERINGMANAGER_H
#define RENDERINGMANAGER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>

class MousePicker;
class LightingManager;
class Window;
class PipelineManager;
class LogicalDevice;
class CommandBuffer;
class SwapChain;

/* Begin Deprecated */
class RenderPass;
class StandardFramebuffer;
class SwapchainFramebuffer;
/* End Deprecated */

class RenderingManager {
public:
  RenderingManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                   const std::shared_ptr<Window>& window,
                   const std::shared_ptr<LightingManager>& lightingManager,
                   const std::shared_ptr<MousePicker>& mousePicker,
                   VkCommandPool commandPool,
                   bool useOffscreenFramebuffer,
                   const char* sceneViewName);

  void recordOffscreenCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t currentFrame,
                                    uint32_t imageIndex) const;

  void recordSwapchainCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t currentFrame,
                                    uint32_t imageIndex) const;

  void doRendering(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t& currentFrame);

  [[nodiscard]] std::shared_ptr<SwapChain> getSwapChain() const;

  void resetOffscreenFramebuffer();

  void setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix);

  [[nodiscard]] std::shared_ptr<RenderPass> getRenderPass() const;

  [[nodiscard]] VkDescriptorSet& getOffscreenImageDescriptorSet(uint32_t imageIndex) const;

  void markFramebufferResized();

  bool isSceneFocused() const;

  void recreateSwapChain();

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<Window> m_window;

  std::shared_ptr<LightingManager> m_lightingManager;

  std::shared_ptr<MousePicker> m_mousePicker;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  std::shared_ptr<CommandBuffer> m_offscreenCommandBuffer;
  std::shared_ptr<CommandBuffer> m_swapchainCommandBuffer;

  std::shared_ptr<SwapChain> m_swapChain;

  glm::vec3 m_viewPosition{};
  glm::mat4 m_viewMatrix{};

  bool m_useOffscreenFramebuffer;

  bool m_framebufferResized = false;

  bool m_sceneIsFocused = false;

  VkExtent2D m_offscreenViewportExtent{};

  ImVec2 m_offscreenViewportPos{0, 0};

  const char* m_sceneViewName;

  /* Begin Deprecated */
  std::shared_ptr<SwapchainFramebuffer> m_framebuffer;
  std::shared_ptr<StandardFramebuffer> m_offscreenFramebuffer;

  std::shared_ptr<RenderPass> m_renderPass;
  std::shared_ptr<RenderPass> m_offscreenRenderPass;
  /* End Deprecated */

  void renderGuiScene(uint32_t imageIndex);
};



#endif //RENDERINGMANAGER_H
