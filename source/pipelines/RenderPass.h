#ifndef VULKANPROJECT_RENDERPASS_H
#define VULKANPROJECT_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>

class RenderPass {
public:
  RenderPass(VkDevice& device, VkPhysicalDevice& physicalDevice, VkFormat imageFormat,
             VkSampleCountFlagBits msaaSamples, VkImageLayout finalLayout);
  ~RenderPass();

  VkRenderPass& getRenderPass();

  [[nodiscard]] VkFormat findDepthFormat() const;

  void begin(const VkFramebuffer& framebuffer, const VkExtent2D& extent, const VkCommandBuffer& commandBuffer) const;
  static void end(const VkCommandBuffer& commandBuffer);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkRenderPass renderPass;

  void createRenderPass(VkFormat imageFormat, VkSampleCountFlagBits msaaSamples, VkImageLayout finalLayout);

  [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};


#endif //VULKANPROJECT_RENDERPASS_H
