#ifndef DYNAMICRENDERER_H
#define DYNAMICRENDERER_H

#include "Renderer.h"

class DynamicRenderer final : public Renderer {
public:
  explicit DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice);

  [[nodiscard]] std::shared_ptr<RenderPass> getRenderPass() const override;

  [[nodiscard]] VkDescriptorSet& getOffscreenImageDescriptorSet(uint32_t imageIndex) const override;

  void resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain) override;

  void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) override;

  void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

  void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                               std::shared_ptr<CommandBuffer> commandBuffer) override;

  void endRendering(std::shared_ptr<CommandBuffer> commandBuffer) override;
};



#endif //DYNAMICRENDERER_H
