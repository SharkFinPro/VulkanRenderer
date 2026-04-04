#include "RenderPass.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include <stdexcept>

namespace vke {

  RenderPass::RenderPass(std::shared_ptr<LogicalDevice> logicalDevice,
                         const RenderPassConfig& renderPassConfig)
    : m_logicalDevice(std::move(logicalDevice)),
      m_shouldClearColorAttachment(renderPassConfig.hasColorAttachment),
      m_shouldClearDepthAttachment(renderPassConfig.hasDepthAttachment)
  {
    createRenderPass(renderPassConfig);
  }

  vk::RenderPass RenderPass::getRenderPass()
  {
    return m_renderPass;
  }

  void RenderPass::begin(const vk::raii::Framebuffer& framebuffer,
                         const vk::Extent2D& extent,
                         const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    std::vector<vk::ClearValue> clearValues;

    if (m_shouldClearColorAttachment)
    {
      clearValues.emplace_back(std::array{0.0f, 0.0f, 0.0f, 1.0f});
    }

    if (m_shouldClearDepthAttachment)
    {
      clearValues.emplace_back(vk::ClearDepthStencilValue{1.0f, 0});
    }

    const vk::RenderPassBeginInfo renderPassInfo {
      .renderPass = *m_renderPass,
      .framebuffer = *framebuffer,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent
      },
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()
    };

    commandBuffer->beginRenderPass(renderPassInfo);
  }

  void RenderPass::createRenderPass(const RenderPassConfig& renderPassConfig)
  {
    const auto attachmentSetup = setupAttachments(renderPassConfig);

    const auto subpass = attachmentSetup.createSubpass();

    constexpr vk::SubpassDependency dependency {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    constexpr uint32_t viewMask = 0x3F;
    constexpr uint32_t correlationMask = 0x3F;

    const vk::RenderPassMultiviewCreateInfo renderPassMultiviewCreateInfo {
      .subpassCount = 1,
      .pViewMasks = &viewMask,
      .correlationMaskCount = 1,
      .pCorrelationMasks = &correlationMask
    };

    const vk::RenderPassCreateInfo renderPassInfo {
      .pNext = renderPassConfig.useMultiview ? &renderPassMultiviewCreateInfo : nullptr,
      .attachmentCount = static_cast<uint32_t>(attachmentSetup.attachments.size()),
      .pAttachments = attachmentSetup.attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency
    };

    m_renderPass = m_logicalDevice->createRenderPass(renderPassInfo);
  }

  AttachmentSetup RenderPass::setupAttachments(const RenderPassConfig& renderPassConfig) const
  {
    AttachmentSetup attachmentSetup;

    uint32_t currentIndex = 0;

    if (renderPassConfig.hasColorAttachment)
    {
      attachmentSetup.attachments.push_back(getColorAttachmentDescription(renderPassConfig));

      attachmentSetup.colorAttachmentReferences.push_back({
        .attachment = currentIndex,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
      });

      currentIndex++;
    }

    if (renderPassConfig.hasDepthAttachment)
    {
      attachmentSetup.attachments.push_back(getDepthAttachmentDescription(renderPassConfig));

      attachmentSetup.depthAttachmentReferences.push_back({
        .attachment = currentIndex,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
      });

      currentIndex++;
    }

    if (renderPassConfig.hasResolveAttachment)
    {
      attachmentSetup.attachments.push_back(getResolveAttachmentDescription(renderPassConfig));

      attachmentSetup.resolveAttachmentReferences.push_back({
        .attachment = currentIndex,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
      });
    }

    return attachmentSetup;
  }

  vk::AttachmentDescription RenderPass::getColorAttachmentDescription(const RenderPassConfig& renderPassConfig)
  {
    return {
      .format = renderPassConfig.imageFormat,
      .samples = renderPassConfig.msaaSamples,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eColorAttachmentOptimal
    };
  }

  vk::AttachmentDescription RenderPass::getDepthAttachmentDescription(const RenderPassConfig& renderPassConfig) const
  {
    return {
      .format = renderPassConfig.depthFormat != vk::Format::eUndefined
                ? renderPassConfig.depthFormat
                : m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .samples = renderPassConfig.msaaSamples,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = renderPassConfig.finalLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal
                 ? vk::AttachmentStoreOp::eStore
                 : vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };
  }

  vk::AttachmentDescription RenderPass::getResolveAttachmentDescription(const RenderPassConfig& renderPassConfig)
  {
    return {
      .format = renderPassConfig.imageFormat,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eDontCare,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = renderPassConfig.finalLayout
    };
  }
} // namespace vke
