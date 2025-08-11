#include "RenderPass.h"
#include "core/commandBuffer/CommandBuffer.h"
#include "core/logicalDevice/LogicalDevice.h"
#include "core/physicalDevice/PhysicalDevice.h"
#include <array>
#include <stdexcept>

RenderPass::RenderPass(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const VkFormat imageFormat,
                       const VkSampleCountFlagBits msaaSamples,
                       const VkImageLayout finalLayout)
  : m_logicalDevice(logicalDevice)
{
  createRenderPass(imageFormat, msaaSamples, finalLayout);
}

RenderPass::~RenderPass()
{
  m_logicalDevice->destroyRenderPass(m_renderPass);
}

VkRenderPass& RenderPass::getRenderPass()
{
  return m_renderPass;
}

void RenderPass::createRenderPass(const VkFormat imageFormat, const VkSampleCountFlagBits msaaSamples,
                                  const VkImageLayout finalLayout)
{
  const VkAttachmentDescription colorAttachment {
    .format = imageFormat,
    .samples = msaaSamples,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  constexpr VkAttachmentReference colorAttachmentRef {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  const VkAttachmentDescription depthAttachment {
    .format = findDepthFormat(),
    .samples = msaaSamples,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };

  constexpr VkAttachmentReference depthAttachmentRef {
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };

  const VkAttachmentDescription colorAttachmentResolve {
    .format = imageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = finalLayout
  };

  constexpr VkAttachmentReference colorAttachmentResolveRef {
    .attachment = 2,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  const VkSubpassDescription subpass {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentRef,
    .pResolveAttachments = finalLayout != VK_IMAGE_LAYOUT_UNDEFINED ? &colorAttachmentResolveRef : nullptr,
    .pDepthStencilAttachment = &depthAttachmentRef
  };

  constexpr VkSubpassDependency dependency {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
  };

  std::vector<VkAttachmentDescription> attachments {
    colorAttachment,
    depthAttachment
  };

  if (finalLayout != VK_IMAGE_LAYOUT_UNDEFINED)
  {
    attachments.push_back(colorAttachmentResolve);
  }

  const VkRenderPassCreateInfo renderPassInfo {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = static_cast<uint32_t>(attachments.size()),
    .pAttachments = attachments.data(),
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };

  m_renderPass = m_logicalDevice->createRenderPass(renderPassInfo);
}

VkFormat RenderPass::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                         VkImageTiling tiling,
                                         VkFormatFeatureFlags features) const
{
  for (const auto& format : candidates)
  {
    const VkFormatProperties formatProperties = m_logicalDevice->getPhysicalDevice()->getFormatProperties(format);

    if ((tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) ||
        (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features))
    {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

VkFormat RenderPass::findDepthFormat() const
{
  return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
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
