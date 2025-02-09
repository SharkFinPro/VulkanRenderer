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
  Framebuffer(std::shared_ptr<PhysicalDevice> physicalDevice,
              std::shared_ptr<LogicalDevice> logicalDevice,
              std::shared_ptr<SwapChain> swapChain,
              const VkCommandPool& commandPool,
              const std::shared_ptr<RenderPass>& renderPass,
              VkExtent2D extent);
  ~Framebuffer();

  VkFramebuffer& getFramebuffer(uint32_t imageIndex);

  VkDescriptorSet& getFramebufferImageDescriptorSet(uint32_t imageIndex);

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<SwapChain> swapChain;

  std::vector<VkFramebuffer> framebuffers;
  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkImage colorImage = VK_NULL_HANDLE;
  VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
  VkImageView colorImageView = VK_NULL_HANDLE;

  VkFormat framebufferImageFormat{};
  std::vector<VkImage> framebufferImages;
  std::vector<VkImageView> framebufferImageViews;
  std::vector<VkDeviceMemory> framebufferImageMemory;

  VkSampler sampler = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> framebufferImageDescriptorSets;

  void createImageResources(const VkCommandPool& commandPool, VkExtent2D extent);
  void createDepthResources(const VkCommandPool& commandPool, VkFormat depthFormat, VkExtent2D extent);
  void createColorResources(VkExtent2D extent);
  void createFrameBuffers(const VkRenderPass& renderPass, VkExtent2D extent);
};


#endif //VULKANPROJECT_FRAMEBUFFER_H
