#ifndef VKE_RENDERPASS_H
#define VKE_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

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

  void begin(const VkFramebuffer& framebuffer,
             const VkExtent2D& extent,
             const std::shared_ptr<CommandBuffer>& commandBuffer) const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkRenderPass m_renderPass = VK_NULL_HANDLE;

  void createRenderPass(VkFormat imageFormat,
                        VkSampleCountFlagBits msaaSamples,
                        VkImageLayout finalLayout);
};

} // namespace vke

#endif //VKE_RENDERPASS_H
