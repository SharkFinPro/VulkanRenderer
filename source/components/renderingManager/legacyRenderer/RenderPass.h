#ifndef VKE_RENDERPASS_H
#define VKE_RENDERPASS_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  struct RenderPassConfig {
    vk::Format imageFormat;
    vk::Format depthFormat = vk::Format::eUndefined;
    vk::SampleCountFlagBits msaaSamples;
    vk::ImageLayout finalLayout;
    bool hasColorAttachment;
    bool hasDepthAttachment;
    bool hasResolveAttachment;
    bool useMultiview = false;
  };

  struct AttachmentSetup {
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> colorAttachmentReferences;
    std::vector<vk::AttachmentReference> depthAttachmentReferences;
    std::vector<vk::AttachmentReference> resolveAttachmentReferences;

    [[nodiscard]] vk::SubpassDescription createSubpass() const
    {
      return {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size()),
        .pColorAttachments = colorAttachmentReferences.empty() ? nullptr : colorAttachmentReferences.data(),
        .pResolveAttachments = resolveAttachmentReferences.empty() ? nullptr : resolveAttachmentReferences.data(),
        .pDepthStencilAttachment = depthAttachmentReferences.empty() ? nullptr : depthAttachmentReferences.data()
      };
    }
  };

  class RenderPass {
  public:
    RenderPass(std::shared_ptr<LogicalDevice> logicalDevice,
               const RenderPassConfig& renderPassConfig);

    [[nodiscard]] const vk::raii::RenderPass& getRenderPass() const;

    void begin(const vk::raii::Framebuffer& framebuffer,
               const vk::Extent2D& extent,
               const std::shared_ptr<CommandBuffer>& commandBuffer) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::RenderPass m_renderPass = nullptr;

    bool m_shouldClearColorAttachment;
    bool m_shouldClearDepthAttachment;

    void createRenderPass(const RenderPassConfig& renderPassConfig);

    [[nodiscard]] AttachmentSetup setupAttachments(const RenderPassConfig& renderPassConfig) const;

    [[nodiscard]] static vk::AttachmentDescription getColorAttachmentDescription(const RenderPassConfig& renderPassConfig);

    [[nodiscard]] vk::AttachmentDescription getDepthAttachmentDescription(const RenderPassConfig& renderPassConfig) const;

    [[nodiscard]] static vk::AttachmentDescription getResolveAttachmentDescription(const RenderPassConfig& renderPassConfig);
  };

} // namespace vke

#endif //VKE_RENDERPASS_H
