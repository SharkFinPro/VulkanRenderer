#ifndef DYNAMICRENDERER_H
#define DYNAMICRENDERER_H

#include "Renderer.h"
#include <vector>

class DynamicRenderer final : public Renderer {
public:
  explicit DynamicRenderer(const std::shared_ptr<LogicalDevice>& logicalDevice, const std::shared_ptr<SwapChain>& swapChain,
                           VkCommandPool commandPool);

  ~DynamicRenderer() override;

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
  size_t m_numImages = 3;

  VkSampler m_sampler = VK_NULL_HANDLE;

  std::vector<VkImage> m_offscreenImages;
  std::vector<VkImageView> m_offscreenImageViews;
  std::vector<VkDeviceMemory> m_offscreenImageMemory;
  std::vector<VkDescriptorSet> m_offscreenImageDescriptorSets;

  std::vector<VkImage> m_swapchainColorImages;
  std::vector<VkImageView> m_swapchainColorImageViews;
  std::vector<VkDeviceMemory> m_swapchainColorImageMemory;

  std::vector<VkImage> m_offscreenColorImages;
  std::vector<VkImageView> m_offscreenColorImageViews;
  std::vector<VkDeviceMemory> m_offscreenColorImageMemory;

  std::vector<VkImage> m_swapchainDepthImages;
  std::vector<VkImageView> m_swapchainDepthImageViews;
  std::vector<VkDeviceMemory> m_swapchainDepthImageMemory;

  std::vector<VkImage> m_offscreenDepthImages;
  std::vector<VkImageView> m_offscreenDepthImageViews;
  std::vector<VkDeviceMemory> m_offscreenDepthImageMemory;

  void createSampler();

  void cleanupSwapchainImageResources();

  void cleanupOffscreenImageResources();

  void createSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain);

  void createOffscreenImageResources(VkExtent2D extent);

  void createColorImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, VkFormat format,
                                VkExtent2D extent) const;

  void createDepthImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory,
                                VkExtent2D extent) const;

  void createImageResource(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory,
                           VkDescriptorSet& imageDescriptorSet, VkExtent2D extent) const;
};



#endif //DYNAMICRENDERER_H
