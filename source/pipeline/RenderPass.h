#ifndef VULKANPROJECT_RENDERPASS_H
#define VULKANPROJECT_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>

class RenderPass {
public:
  RenderPass(VkDevice& device, VkPhysicalDevice& physicalDevice, VkFormat swapChainImageFormat,
             VkSampleCountFlagBits msaaSamples, VkFormat depthFormat);
  ~RenderPass();

  VkRenderPass& getRenderPass();

private:

  void createRenderPass(VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples, VkFormat depthFormat);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkRenderPass renderPass;
};


#endif //VULKANPROJECT_RENDERPASS_H
