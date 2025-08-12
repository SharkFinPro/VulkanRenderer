#ifndef LEGACYRENDERER_H
#define LEGACYRENDERER_H

#include "Renderer.h"
#include <memory>

class RenderPass;
class StandardFramebuffer;
class SwapChain;
class SwapchainFramebuffer;

class LegacyRenderer final : public Renderer {
public:
  LegacyRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice, std::shared_ptr<SwapChain> swapChain,
                 VkCommandPool commandPool);

  [[nodiscard]] std::shared_ptr<RenderPass> getRenderPass() const override;

  [[nodiscard]] VkDescriptorSet& getOffscreenImageDescriptorSet(uint32_t imageIndex) override;

  void resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain) override;

  void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) override;

  void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

  void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

  void endRendering(std::shared_ptr<CommandBuffer> commandBuffer) override;

private:
  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  std::shared_ptr<SwapchainFramebuffer> m_framebuffer;
  std::shared_ptr<StandardFramebuffer> m_offscreenFramebuffer;

  std::shared_ptr<RenderPass> m_renderPass;
  std::shared_ptr<RenderPass> m_offscreenRenderPass;
};



#endif //LEGACYRENDERER_H
