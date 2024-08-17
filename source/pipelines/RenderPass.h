#ifndef VULKANPROJECT_RENDERPASS_H
#define VULKANPROJECT_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>

class RenderPass {
public:
  RenderPass(VkDevice& device, VkPhysicalDevice& physicalDevice, VkFormat swapChainImageFormat,
             VkSampleCountFlagBits msaaSamples);
  ~RenderPass();

  VkRenderPass& getRenderPass();

  VkFormat findDepthFormat();

  void begin(const VkFramebuffer& framebuffer, const VkExtent2D& extent, const VkCommandBuffer& commandBuffer);
  static void end(const VkCommandBuffer& commandBuffer);

private:
  void createRenderPass(VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples);
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkRenderPass renderPass;
};


#endif //VULKANPROJECT_RENDERPASS_H
