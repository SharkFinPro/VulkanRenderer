#ifndef SWAPCHAINFRAMEBUFFER_H
#define SWAPCHAINFRAMEBUFFER_H

#include "Framebuffer.h"

class SwapChain;

class SwapchainFramebuffer final : public Framebuffer {
public:
  SwapchainFramebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const std::shared_ptr<SwapChain>& swapChain,
                       const VkCommandPool& commandPool,
                       const std::shared_ptr<RenderPass>& renderPass,
                       VkExtent2D extent);

private:
  std::shared_ptr<SwapChain> m_swapChain;

  [[nodiscard]] VkFormat getColorFormat() override;

  [[nodiscard]] const std::vector<VkImageView>& getImageViews() override;
};

#endif //SWAPCHAINFRAMEBUFFER_H
