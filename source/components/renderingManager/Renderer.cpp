#include "Renderer.h"
#include "ImageResource.h"
#include "RenderTarget.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../window/SwapChain.h"

namespace vke {

  Renderer::Renderer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const std::shared_ptr<SwapChain>& swapChain,
                     VkCommandPool commandPool)
    : m_logicalDevice(logicalDevice), m_commandPool(commandPool)
  {
    createSampler();

    createSwapchainRenderTarget(swapChain);
  }

  Renderer::~Renderer()
  {
    m_logicalDevice->destroySampler(m_sampler);
  }

  VkDescriptorSet Renderer::getOffscreenImageDescriptorSet(uint32_t imageIndex)
  {
    if (!m_offscreenRenderTarget)
    {
      return VK_NULL_HANDLE;
    }

    return m_offscreenRenderTarget->getResolveImageResource(imageIndex).getDescriptorSet();
  }

  void Renderer::resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain)
  {
    m_swapchainRenderTarget.reset();

    createSwapchainRenderTarget(swapChain);
  }

  void Renderer::resetOffscreenImageResources(const VkExtent2D offscreenViewportExtent)
  {
    m_offscreenRenderTarget.reset();

    createOffscreenRenderTarget(offscreenViewportExtent);
  }

  void Renderer::createSampler()
  {
    constexpr VkSamplerCreateInfo samplerInfo {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE
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

    m_swapchainRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig);
  }

  void Renderer::createOffscreenRenderTarget(const VkExtent2D extent)
  {
    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = extent,
      .commandPool = m_commandPool,
      .colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .resolveFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .numSamples = m_logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .sampler = m_sampler
    };

    m_offscreenRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig);
  }
} // namespace vke