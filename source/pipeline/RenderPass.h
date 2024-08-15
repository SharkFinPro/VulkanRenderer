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

private:
  void createRenderPass(VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples);
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkRenderPass renderPass;
};


#endif //VULKANPROJECT_RENDERPASS_H
