#ifndef VKE_LEGACYRENDERER_H
#define VKE_LEGACYRENDERER_H

#include "../Renderer.h"
#include <memory>

namespace vke {

class RenderPass;
class StandardFramebuffer;
class SwapChain;
class SwapchainFramebuffer;

class LegacyRenderer final : public Renderer {
public:
  LegacyRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice, const std::shared_ptr<SwapChain>& swapChain,
                 VkCommandPool commandPool);

  [[nodiscard]] std::shared_ptr<RenderPass> getRenderPass() const override;

  [[nodiscard]] VkDescriptorSet getOffscreenImageDescriptorSet(uint32_t imageIndex) override;

  void resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain) override;

  void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) override;

  void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer,
                               std::shared_ptr<SwapChain> swapChain) override;

  void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

  void endSwapchainRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer,
                             std::shared_ptr<SwapChain> swapChain) override;

  void endOffscreenRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer) override;

private:
  std::shared_ptr<SwapchainFramebuffer> m_framebuffer;
  std::shared_ptr<StandardFramebuffer> m_offscreenFramebuffer;

  std::shared_ptr<RenderPass> m_renderPass;
  std::shared_ptr<RenderPass> m_offscreenRenderPass;

  static void endRendering(const std::shared_ptr<CommandBuffer>& commandBuffer);
};

} // namespace vke

#endif //VKE_LEGACYRENDERER_H
