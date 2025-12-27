#include "RenderPass.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include <array>
#include <stdexcept>

namespace vke {

  RenderPass::RenderPass(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const RenderPassConfig& renderPassConfig)
    : m_logicalDevice(logicalDevice)
  {
    createRenderPass(renderPassConfig);
  }

  RenderPass::~RenderPass()
  {
    m_logicalDevice->destroyRenderPass(m_renderPass);
  }

  VkRenderPass& RenderPass::getRenderPass()
  {
    return m_renderPass;
  }

  void RenderPass::begin(const VkFramebuffer& framebuffer, const VkExtent2D& extent,
                         const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    constexpr std::array<VkClearValue, 2> clearValues {{
      {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
      {.depthStencil = {1.0f, 0}}
    }};

    const VkRenderPassBeginInfo renderPassInfo {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = m_renderPass,
      .framebuffer = framebuffer,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
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

    constexpr VkSubpassDependency dependency {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    const VkRenderPassCreateInfo renderPassInfo {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
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
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });

      currentIndex++;
    }

    if (renderPassConfig.hasDepthAttachment)
    {
      attachmentSetup.attachments.push_back(getDepthAttachmentDescription(renderPassConfig));

      attachmentSetup.depthAttachmentReferences.push_back({
        .attachment = currentIndex,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      });

      currentIndex++;
    }

    if (renderPassConfig.hasResolveAttachment)
    {
      attachmentSetup.attachments.push_back(getResolveAttachmentDescription(renderPassConfig));

      attachmentSetup.resolveAttachmentReferences.push_back({
        .attachment = currentIndex,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });
    }

    return attachmentSetup;
  }

  VkAttachmentDescription RenderPass::getColorAttachmentDescription(const RenderPassConfig& renderPassConfig)
  {
    const VkAttachmentDescription colorAttachmentDescription {
      .format = renderPassConfig.imageFormat,
      .samples = renderPassConfig.msaaSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    return colorAttachmentDescription;
  }

  VkAttachmentDescription RenderPass::getDepthAttachmentDescription(const RenderPassConfig& renderPassConfig) const
  {
    const VkAttachmentDescription depthAttachmentDescription {
      .format = renderPassConfig.depthFormat != VK_FORMAT_UNDEFINED
                ? renderPassConfig.depthFormat
                : m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .samples = renderPassConfig.msaaSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = renderPassConfig.finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                 ? VK_ATTACHMENT_STORE_OP_STORE
                 : VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    return depthAttachmentDescription;
  }

  VkAttachmentDescription RenderPass::getResolveAttachmentDescription(const RenderPassConfig& renderPassConfig)
  {
    const VkAttachmentDescription resolveAttachmentDescription {
      .format = renderPassConfig.imageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = renderPassConfig.finalLayout
    };

    return resolveAttachmentDescription;
  }
} // namespace vke
