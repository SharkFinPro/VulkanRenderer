#ifndef VKE_RENDERPASS_H
#define VKE_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  struct RenderPassConfig {
    VkFormat imageFormat;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits msaaSamples;
    VkImageLayout finalLayout;
    bool hasColorAttachment;
    bool hasDepthAttachment;
    bool hasResolveAttachment;
  };

  struct AttachmentSetup {
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentReferences;
    std::vector<VkAttachmentReference> depthAttachmentReferences;
    std::vector<VkAttachmentReference> resolveAttachmentReferences;

    [[nodiscard]] VkSubpassDescription createSubpass() const
    {
      return {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size()),
        .pColorAttachments = colorAttachmentReferences.empty() ? nullptr : colorAttachmentReferences.data(),
        .pResolveAttachments = resolveAttachmentReferences.empty() ? nullptr : resolveAttachmentReferences.data(),
        .pDepthStencilAttachment = depthAttachmentReferences.empty() ? nullptr : depthAttachmentReferences.data()
      };
    }
  };

  class RenderPass {
  public:
    RenderPass(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const RenderPassConfig& renderPassConfig);
    ~RenderPass();

    VkRenderPass& getRenderPass();

    void begin(const VkFramebuffer& framebuffer,
               const VkExtent2D& extent,
               const std::shared_ptr<CommandBuffer>& commandBuffer) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    bool m_shouldClearColorAttachment;
    bool m_shouldClearDepthAttachment;

    void createRenderPass(const RenderPassConfig& renderPassConfig);

    [[nodiscard]] AttachmentSetup setupAttachments(const RenderPassConfig& renderPassConfig) const;

    [[nodiscard]] static VkAttachmentDescription getColorAttachmentDescription(const RenderPassConfig& renderPassConfig);

    [[nodiscard]] VkAttachmentDescription getDepthAttachmentDescription(const RenderPassConfig& renderPassConfig) const;

    [[nodiscard]] static VkAttachmentDescription getResolveAttachmentDescription(const RenderPassConfig& renderPassConfig);
  };

} // namespace vke

#endif //VKE_RENDERPASS_H
