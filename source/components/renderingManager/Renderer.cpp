#include "Renderer.h"
#include "ImageResource.h"
#include "RenderTarget.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../window/SwapChain.h"

namespace vke {

  Renderer::Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                     const std::shared_ptr<SwapChain>& swapChain,
                     const vk::CommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    createSampler();

    createSwapchainRenderTarget(swapChain);

    createMousePickingRenderTarget(swapChain->getExtent());
  }

  vk::DescriptorSet Renderer::getOffscreenImageDescriptorSet(const uint32_t imageIndex)
  {
    if (!m_offscreenRenderTarget)
    {
      return VK_NULL_HANDLE;
    }

    return m_offscreenRenderTarget->getResolveImageResource(imageIndex).getDescriptorSet();
  }

  std::shared_ptr<RenderTarget> Renderer::getMousePickingRenderTarget() const
  {
    return m_mousePickingRenderTarget;
  }

  void Renderer::resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain)
  {
    m_swapchainRenderTarget.reset();

    createSwapchainRenderTarget(swapChain);
  }

  void Renderer::resetOffscreenImageResources(const vk::Extent2D offscreenViewportExtent)
  {
    m_offscreenRenderTarget.reset();

    createOffscreenRenderTarget(offscreenViewportExtent);

    resetRayTracingImageResources(offscreenViewportExtent);
  }

  void Renderer::resetMousePickingImageResources(const vk::Extent2D mousePickingExtent)
  {
    m_mousePickingRenderTarget.reset();

    createMousePickingRenderTarget(mousePickingExtent);
  }

  void Renderer::resetRayTracingImageResources(vk::Extent2D extent)
  {
    if (!supportsRayTracing())
    {
      return;
    }

    m_rayTracingImageResources.clear();

    createRayTracingImageResource(extent);
  }

  uint32_t Renderer::registerShadowMapRenderTarget([[maybe_unused]] std::shared_ptr<RenderTarget> renderTarget,
                                                   [[maybe_unused]] bool isCubeMap)
  {
    return ++m_currentShadowMapRenderTargetID;
  }

  std::shared_ptr<ImageResource> Renderer::getRayTracingImageResource(const uint32_t currentFrame) const
  {
    return m_rayTracingImageResources.at(currentFrame);
  }

  void Renderer::createSampler()
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

  void Renderer::createSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = swapChain->getExtent(),
      .commandPool = m_commandPool,
      .colorFormat = swapChain->getImageFormat(),
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples()
    };

    m_swapchainRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, static_cast<uint32_t>(swapChain->getImages().size()));
  }

  void Renderer::createOffscreenRenderTarget(const vk::Extent2D extent)
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

    m_offscreenRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, m_logicalDevice->getMaxFramesInFlight());
  }

  void Renderer::createMousePickingRenderTarget(const vk::Extent2D extent)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = vk::Format::eR8G8B8A8Uint,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = vk::SampleCountFlagBits::e1
    };

    m_mousePickingRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig, m_logicalDevice->getMaxFramesInFlight());
  }

  void Renderer::createRayTracingImageResource(const vk::Extent2D extent)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }
    
    ImageResourceConfig imageResourceConfig {
      .imageResourceType = ImageResourceType::RayTracingOutput,
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .resolveFormat = vk::Format::eR8G8B8A8Unorm,
      .numSamples = vk::SampleCountFlagBits::e1
    };

    for (size_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); ++i)
    {
      m_rayTracingImageResources.emplace_back(std::make_shared<ImageResource>(imageResourceConfig));
    }
  }
} // namespace vke