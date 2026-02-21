#include "DynamicRenderer.h"
#include "../ImageResource.h"
#include "../RenderTarget.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/lights/Light.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../window/SwapChain.h"
#include "../../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {

  DynamicRenderer::DynamicRenderer(std::shared_ptr<LogicalDevice> logicalDevice,
                                   const std::shared_ptr<SwapChain>& swapChain,
                                   VkCommandPool commandPool)
    : Renderer(std::move(logicalDevice), swapChain, commandPool)
  {}

  void DynamicRenderer::beginSwapchainRendering(const uint32_t imageIndex,
                                                const VkExtent2D extent,
                                                const std::shared_ptr<CommandBuffer> commandBuffer,
                                                const std::shared_ptr<SwapChain> swapChain)
  {
    transitionSwapchainImagePreRender(commandBuffer, swapChain->getImages()[imageIndex]);

    VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_swapchainRenderTarget->getColorImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
      .resolveImageView = swapChain->getImageViews()[imageIndex],
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_swapchainRenderTarget->getDepthImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {
        .depthStencil = {1.0f, 0}
      }
    };

    const VkRenderingInfo renderingInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginOffscreenRendering(const uint32_t imageIndex,
                                                const VkExtent2D extent,
                                                const std::shared_ptr<CommandBuffer> commandBuffer)
  {
    VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_offscreenRenderTarget->getColorImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
      .resolveImageView = m_offscreenRenderTarget->getResolveImageResource(imageIndex).getImageView(),
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_offscreenRenderTarget->getDepthImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {
        .depthStencil = {1.0f, 0}
      }
    };

    const VkRenderingInfo renderingInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginShadowRendering(uint32_t imageIndex,
                                             const VkExtent2D extent,
                                             const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const std::shared_ptr<Light>& light)
  {
    VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = light->getShadowMapView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {
        .depthStencil = {1.0f, 0}
      }
    };

    const VkRenderingInfo renderingInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .viewMask = light->getLightType() == LightType::pointLight ? 0x3Fu : 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::beginMousePickingRendering(const uint32_t imageIndex,
                                                   const VkExtent2D extent,
                                                   const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    VkRenderingAttachmentInfo colorRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_mousePickingRenderTarget->getColorImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    VkRenderingAttachmentInfo depthRenderingAttachmentInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = m_mousePickingRenderTarget->getDepthImageResource(imageIndex).getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {
        .depthStencil = {1.0f, 0}
      }
    };

    const VkRenderingInfo renderingInfo {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {
        .offset = {0, 0},
        .extent = extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void DynamicRenderer::endSwapchainRendering(const uint32_t imageIndex,
                                              const std::shared_ptr<CommandBuffer> commandBuffer,
                                              const std::shared_ptr<SwapChain> swapChain)
  {
    commandBuffer->endRendering();

    transitionSwapchainImagePostRender(commandBuffer, swapChain->getImages()[imageIndex]);
  }

  void DynamicRenderer::endOffscreenRendering(const std::shared_ptr<CommandBuffer> commandBuffer)
  {
    commandBuffer->endRendering();
  }

  void DynamicRenderer::endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    commandBuffer->endRendering();
  }

  void DynamicRenderer::endMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    commandBuffer->endRendering();
  }

  void DynamicRenderer::beginRayTracingRendering(const uint32_t imageIndex,
                                                 const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    const VkImageMemoryBarrier imageMemoryBarrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_NONE,
      .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = m_rayTracingImageResource->getImage(),
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    commandBuffer->pipelineBarrier(
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
      0,
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void DynamicRenderer::endRayTracingRendering(const uint32_t imageIndex,
                                               const std::shared_ptr<CommandBuffer>& commandBuffer)
  {
    // TODO: Transition & copy image
  }

  void DynamicRenderer::transitionSwapchainImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                          const VkImage image)
  {
    const VkImageMemoryBarrier imageMemoryBarrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_NONE,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .image = image,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };

    commandBuffer->pipelineBarrier(
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      0,
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void DynamicRenderer::transitionSwapchainImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                           const VkImage image)
  {
    const VkImageMemoryBarrier imageMemoryBarrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_NONE,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = image,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };

    commandBuffer->pipelineBarrier(
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      0,
      {},
      {},
      { imageMemoryBarrier }
    );
  }

} // namespace vke