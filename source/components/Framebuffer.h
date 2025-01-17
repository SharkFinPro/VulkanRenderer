#ifndef VULKANPROJECT_FRAMEBUFFER_H
#define VULKANPROJECT_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "../pipelines/RenderPass.h"

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
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;

  VkFormat framebufferImageFormat;
  std::vector<VkImage> framebufferImages;
  std::vector<VkImageView> framebufferImageViews;
  std::vector<VkDeviceMemory> framebufferImageMemory;

  VkSampler sampler;
  std::vector<VkDescriptorSet> framebufferImageDescriptorSets;

  void createImageResources(const VkCommandPool& commandPool, VkExtent2D extent);
  void createDepthResources(const VkCommandPool& commandPool, VkFormat depthFormat, VkExtent2D extent);
  void createColorResources(VkExtent2D extent);
  void createFrameBuffers(const VkRenderPass& renderPass, VkExtent2D extent);
};


#endif //VULKANPROJECT_FRAMEBUFFER_H
