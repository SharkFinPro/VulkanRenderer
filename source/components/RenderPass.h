#ifndef VULKANPROJECT_RENDERPASS_H
#define VULKANPROJECT_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class CommandBuffer;
class LogicalDevice;

class RenderPass {
public:
  RenderPass(const std::shared_ptr<LogicalDevice>& logicalDevice,
             VkFormat imageFormat,
             VkSampleCountFlagBits msaaSamples,
             VkImageLayout finalLayout);
  ~RenderPass();

  VkRenderPass& getRenderPass();

  [[nodiscard]] VkFormat findDepthFormat() const;

  void begin(const VkFramebuffer& framebuffer, const VkExtent2D& extent, const std::shared_ptr<CommandBuffer>& commandBuffer) const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkRenderPass m_renderPass = VK_NULL_HANDLE;

  void createRenderPass(VkFormat imageFormat, VkSampleCountFlagBits msaaSamples, VkImageLayout finalLayout);

  [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};


#endif //VULKANPROJECT_RENDERPASS_H
