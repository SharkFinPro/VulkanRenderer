#ifndef VULKANPROJECT_FRAMEBUFFER_H
#define VULKANPROJECT_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class LogicalDevice;
class PhysicalDevice;
class RenderPass;
class SwapChain;

class Framebuffer {
public:
  Framebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const std::shared_ptr<SwapChain>& swapChain,
              const VkCommandPool& commandPool,
              const std::shared_ptr<RenderPass>& renderPass,
              VkExtent2D extent,
              bool mousePicking = false);
  ~Framebuffer();

  VkFramebuffer& getFramebuffer(uint32_t imageIndex);

  VkDescriptorSet& getFramebufferImageDescriptorSet(uint32_t imageIndex);

  VkImage& getColorImage();

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;
  std::shared_ptr<SwapChain> m_swapChain;

  std::vector<VkFramebuffer> m_framebuffers;
  VkImage m_depthImage = VK_NULL_HANDLE;
  VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
  VkImageView m_depthImageView = VK_NULL_HANDLE;
  VkImage m_colorImage = VK_NULL_HANDLE;
  VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
  VkImageView m_colorImageView = VK_NULL_HANDLE;

  VkFormat m_framebufferImageFormat{};
  std::vector<VkImage> m_framebufferImages;
  std::vector<VkImageView> m_framebufferImageViews;
  std::vector<VkDeviceMemory> m_framebufferImageMemory;

  VkSampler m_sampler = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_framebufferImageDescriptorSets;

  bool m_mousePicking;

  void createImageResources(const VkCommandPool& commandPool, VkExtent2D extent);
  void createDepthResources(const VkCommandPool& commandPool, VkFormat depthFormat, VkExtent2D extent);
  void createColorResources(VkExtent2D extent);
  void createFrameBuffers(const VkRenderPass& renderPass, VkExtent2D extent);
};


#endif //VULKANPROJECT_FRAMEBUFFER_H
