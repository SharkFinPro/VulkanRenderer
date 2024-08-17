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
              VkCommandPool& commandPool,
              const std::shared_ptr<RenderPass>& renderPass);
  ~Framebuffer();

  VkFramebuffer& getFramebuffer(uint32_t imageIndex);

private:
  void createDepthResources(VkCommandPool& commandPool, VkFormat depthFormat);
  void createColorResources();
  void createFrameBuffers(VkRenderPass& renderPass);

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
};


#endif //VULKANPROJECT_FRAMEBUFFER_H
