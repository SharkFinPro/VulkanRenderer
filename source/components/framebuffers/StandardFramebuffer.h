#ifndef STANDARDFRAMEBUFFER_H
#define STANDARDFRAMEBUFFER_H

#include "Framebuffer.h"

class StandardFramebuffer final : public Framebuffer {
public:
  StandardFramebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const VkCommandPool& commandPool,
                     const std::shared_ptr<RenderPass>& renderPass,
                     VkExtent2D extent,
                     bool mousePicking = false);

  ~StandardFramebuffer() override;

  VkDescriptorSet& getFramebufferImageDescriptorSet(uint32_t imageIndex);

private:
  VkCommandPool m_commandPool;
  VkExtent2D m_extent;

  VkFormat m_framebufferImageFormat{};
  std::vector<VkImage> m_framebufferImages;
  std::vector<VkImageView> m_framebufferImageViews;
  std::vector<VkDeviceMemory> m_framebufferImageMemory;

  VkSampler m_sampler = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_framebufferImageDescriptorSets;

  [[nodiscard]] VkFormat getColorFormat() override;

  [[nodiscard]] const std::vector<VkImageView>& getImageViews() override;

  void createSampler();

  void createImageResources();

  void createImageResource(size_t imageIndex);
};

#endif //STANDARDFRAMEBUFFER_H
