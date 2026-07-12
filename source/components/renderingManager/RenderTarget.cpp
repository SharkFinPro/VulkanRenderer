#include "RenderTarget.h"
#include "ImageResource.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"

namespace vke {

  RenderTarget::RenderTarget(std::shared_ptr<LogicalDevice> logicalDevice,
                             const vk::CommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    createSampler();

    createDescriptorPool();

    createOffscreenImageDescriptorSet();
  }

  ImageResource& RenderTarget::getOffscreenResolveImageResource(const uint32_t currentFrame)
  {
    return m_offscreenResolveImageResources.at(currentFrame);
  }

  ImageResource& RenderTarget::getOffscreenRayTracingImageResource(const uint32_t currentFrame)
  {
    return m_offscreenRayTracingImageResources.at(currentFrame);
  }

  ImageResource& RenderTarget::getMousePickingColorImageResource(const uint32_t currentFrame)
  {
    return m_mousePickingColorImageResources.at(currentFrame);
  }

  vk::DescriptorSetLayout RenderTarget::getOffscreenImageDescriptorSetLayout() const
  {
    return m_offscreenImageDescriptorSet->getDescriptorSetLayout();
  }

  vk::DescriptorSet RenderTarget::getOffscreenImageDescriptorSet(const uint32_t currentFrame) const
  {
    return m_offscreenImageDescriptorSet->getDescriptorSet(currentFrame);
  }

  void RenderTarget::recreateImageResources(const vk::Extent2D extent)
  {
    m_offscreenColorImageResources.clear();
    m_offscreenDepthImageResources.clear();
    m_offscreenResolveImageResources.clear();

    m_offscreenRayTracingImageResources.clear();

    m_mousePickingColorImageResources.clear();
    m_mousePickingDepthImageResources.clear();

    m_extent = extent;

    createOffscreenImageResources(extent);
    createMousePickingImageResources(extent);

    m_offscreenImageDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<vk::WriteDescriptorSet> descriptorWrites {{
        {
          .dstSet = descriptorSet,
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eCombinedImageSampler,
          .pImageInfo = &m_offscreenResolveImageResources.at(frame).getDescriptorImageInfo()
        }
      }};

      return descriptorWrites;
    });
  }

  void RenderTarget::beginOffscreenRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const uint32_t currentFrame) const
  {
    // The resolve image was last sampled as SHADER_READ_ONLY_OPTIMAL (or is in its initial
    // layout). Its contents are fully overwritten by the resolve, so discard them.
    const vk::ImageMemoryBarrier2 resolveImageBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
      .srcAccessMask = vk::AccessFlagBits2::eNone,
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = m_offscreenResolveImageResources.at(currentFrame).getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &resolveImageBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);

    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_offscreenColorImageResources.at(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eAverage,
      .resolveImageView = m_offscreenResolveImageResources.at(currentFrame).getImageView(),
      .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_offscreenDepthImageResources.at(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void RenderTarget::endOffscreenRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                           const uint32_t currentFrame) const
  {
    commandBuffer->endRendering();

    // Hand the resolved image over to the swapchain pass, which samples it in the fragment
    // shader (ImGui scene view or the offscreenToSwapchain pipeline).
    const vk::ImageMemoryBarrier2 resolveImageBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = m_offscreenResolveImageResources.at(currentFrame).getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &resolveImageBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void RenderTarget::beginMousePickingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                const uint32_t currentFrame) const
  {
    vk::RenderingAttachmentInfo colorRenderingAttachmentInfo {
      .imageView = m_mousePickingColorImageResources.at(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eNone,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = s_clearColor
    };

    vk::RenderingAttachmentInfo depthRenderingAttachmentInfo {
      .imageView = m_mousePickingDepthImageResources.at(currentFrame).getImageView(),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = s_clearDepth
    };

    const vk::RenderingInfo renderingInfo {
      .renderArea = {
        .offset = {0, 0},
        .extent = m_extent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRenderingAttachmentInfo,
      .pDepthAttachment = &depthRenderingAttachmentInfo,
    };

    commandBuffer->beginRendering(renderingInfo);
  }

  void RenderTarget::beginRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                              const uint32_t currentFrame) const
  {
    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eNone,
      .srcAccessMask = vk::AccessFlagBits2::eNone,
      .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
      .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eGeneral,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = m_offscreenRayTracingImageResources.at(currentFrame).getImage(),
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void RenderTarget::endRayTracingRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                            const uint32_t currentFrame) const
  {
    transitionRayTracingImagePreCopy(commandBuffer, currentFrame);

    copyRayTracingImageToOffscreenImage(commandBuffer, currentFrame);

    transitionRayTracingImagePostCopy(commandBuffer, currentFrame);
  }

  void RenderTarget::createSampler()
  {
    constexpr vk::SamplerCreateInfo samplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eClampToEdge,
      .addressModeV = vk::SamplerAddressMode::eClampToEdge,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::False,
      .maxAnisotropy = 1.0f,
      .compareEnable = vk::False,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = vk::False
    };

    m_sampler = m_logicalDevice->createSampler(samplerInfo);
  }

  void RenderTarget::createDescriptorPool()
  {
    const std::array<vk::DescriptorPoolSize, 1> poolSizes {{
      { vk::DescriptorType::eCombinedImageSampler, m_logicalDevice->getMaxFramesInFlight() }
    }};

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .maxSets = m_logicalDevice->getMaxFramesInFlight(),
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
  }

  void RenderTarget::createOffscreenImageDescriptorSet()
  {
    const std::vector<vk::DescriptorSetLayoutBinding> layoutBindings {
      {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
      }
    };

    m_offscreenImageDescriptorSet = std::make_unique<DescriptorSet>(m_logicalDevice, m_descriptorPool, layoutBindings);
  }

  void RenderTarget::createOffscreenImageResources(vk::Extent2D extent)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = vk::Format::eR8G8B8A8Unorm,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .resolveFormat = vk::Format::eR8G8B8A8Unorm,
      .numSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .sampler = m_sampler
    };

    const auto numImages = m_logicalDevice->getMaxFramesInFlight();

    if (m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      auto rayTracingImageResourceConfig = imageResourceConfig;
      rayTracingImageResourceConfig.imageResourceType = ImageResourceType::RayTracingOutput;
      rayTracingImageResourceConfig.rayTracingFormat = vk::Format::eR8G8B8A8Unorm;
      rayTracingImageResourceConfig.numSamples = vk::SampleCountFlagBits::e1;

      m_offscreenRayTracingImageResources.reserve(numImages);

      for (size_t i = 0; i < numImages; ++i)
      {
        m_offscreenRayTracingImageResources.emplace_back(rayTracingImageResourceConfig);
      }
    }

    auto colorImageResourceConfig = imageResourceConfig;
    colorImageResourceConfig.imageResourceType = ImageResourceType::Color;

    auto depthImageResourceConfig = imageResourceConfig;
    depthImageResourceConfig.imageResourceType = ImageResourceType::Depth;

    auto resolveImageResourceConfig = imageResourceConfig;
    resolveImageResourceConfig.imageResourceType = ImageResourceType::Resolve;
    resolveImageResourceConfig.numSamples = vk::SampleCountFlagBits::e1;

    m_offscreenColorImageResources.reserve(numImages);
    m_offscreenDepthImageResources.reserve(numImages);
    m_offscreenResolveImageResources.reserve(numImages);

    for (size_t i = 0; i < numImages; ++i)
    {
      m_offscreenColorImageResources.emplace_back(colorImageResourceConfig);
      m_offscreenDepthImageResources.emplace_back(depthImageResourceConfig);
      m_offscreenResolveImageResources.emplace_back(resolveImageResourceConfig);
    }
  }

  void RenderTarget::createMousePickingImageResources(const vk::Extent2D extent)
  {
    const ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = vk::Format::eR8G8B8A8Uint,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = vk::SampleCountFlagBits::e1
    };

    auto colorImageResourceConfig = imageResourceConfig;
    colorImageResourceConfig.imageResourceType = ImageResourceType::Color;

    auto depthImageResourceConfig = imageResourceConfig;
    depthImageResourceConfig.imageResourceType = ImageResourceType::Depth;

    const auto numImages = m_logicalDevice->getMaxFramesInFlight();

    m_mousePickingColorImageResources.reserve(numImages);
    m_mousePickingDepthImageResources.reserve(numImages);

    for (size_t i = 0; i < numImages; ++i)
    {
      m_mousePickingColorImageResources.emplace_back(colorImageResourceConfig);
      m_mousePickingDepthImageResources.emplace_back(depthImageResourceConfig);
    }
  }

  void RenderTarget::transitionRayTracingImagePreCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                      const uint32_t currentFrame) const
  {
    const auto rtImage = m_offscreenRayTracingImageResources.at(currentFrame).getImage();
    const auto offscreenImage = m_offscreenResolveImageResources.at(currentFrame).getImage();

    // Transition RT image: GENERAL -> TRANSFER_SRC_OPTIMAL
    // Transition offscreen resolve image (last sampled in a fragment shader; contents
    // discarded): UNDEFINED -> TRANSFER_DST_OPTIMAL
    const std::array preTransferBarriers {
      vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
        .oldLayout = vk::ImageLayout::eGeneral,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = rtImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      },
      vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = offscreenImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = static_cast<uint32_t>(preTransferBarriers.size()),
      .pImageMemoryBarriers = preTransferBarriers.data()
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void RenderTarget::transitionRayTracingImagePostCopy(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                       const uint32_t currentFrame) const
  {
    const auto rtImage = m_offscreenRayTracingImageResources.at(currentFrame).getImage();
    const auto offscreenImage = m_offscreenResolveImageResources.at(currentFrame).getImage();

    // Transition offscreen resolve: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    // Transition RT image back: TRANSFER_SRC_OPTIMAL -> GENERAL (its next use re-transitions
    // from UNDEFINED, so no destination scope is needed)
    const std::array postTransferBarriers {
      vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = offscreenImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      },
      vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
        .dstStageMask = vk::PipelineStageFlagBits2::eNone,
        .dstAccessMask = vk::AccessFlagBits2::eNone,
        .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = rtImage,
        .subresourceRange = {
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
        }
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = static_cast<uint32_t>(postTransferBarriers.size()),
      .pImageMemoryBarriers = postTransferBarriers.data()
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void RenderTarget::copyRayTracingImageToOffscreenImage(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                         const uint32_t currentFrame) const
  {
    const auto rtImage = m_offscreenRayTracingImageResources.at(currentFrame).getImage();
    const auto offscreenImage = m_offscreenResolveImageResources.at(currentFrame).getImage();

    const vk::ImageCopy imageCopy {
      .srcSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
      .srcOffset = { 0, 0, 0 },
      .dstSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
      .dstOffset = { 0, 0, 0 },
      .extent = { m_extent.width, m_extent.height, 1 }
    };

    commandBuffer->copyImage(
      rtImage,
      vk::ImageLayout::eTransferSrcOptimal,
      offscreenImage,
      vk::ImageLayout::eTransferDstOptimal,
      { imageCopy }
    );
  }

} // namespace vke