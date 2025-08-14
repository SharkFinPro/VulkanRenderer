#ifndef VULKANPROJECT_FRAMEBUFFER_H
#define VULKANPROJECT_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

class LogicalDevice;
class RenderPass;

class Framebuffer {
public:
  explicit Framebuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       bool mousePicking = false);

  virtual ~Framebuffer();

  void initializeFramebuffer(const VkCommandPool& commandPool,
                             const std::shared_ptr<RenderPass>& renderPass,
                             VkExtent2D extent);

  VkFramebuffer& getFramebuffer(uint32_t imageIndex);

  VkImage& getColorImage();

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::vector<VkFramebuffer> m_framebuffers;
  VkImage m_depthImage = VK_NULL_HANDLE;
  VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
  VkImageView m_depthImageView = VK_NULL_HANDLE;
  VkImage m_colorImage = VK_NULL_HANDLE;
  VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
  VkImageView m_colorImageView = VK_NULL_HANDLE;

  bool m_mousePicking;

  void createDepthResources(const VkCommandPool& commandPool, VkFormat depthFormat, VkExtent2D extent);
  void createColorResources(VkExtent2D extent);
  void createFrameBuffers(const VkRenderPass& renderPass, VkExtent2D extent);

  [[nodiscard]] virtual VkFormat getColorFormat() = 0;
  [[nodiscard]] virtual const std::vector<VkImageView>& getImageViews() = 0;
};

} // namespace vke

#endif //VULKANPROJECT_FRAMEBUFFER_H
