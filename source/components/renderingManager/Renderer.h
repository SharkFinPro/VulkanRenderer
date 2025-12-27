#ifndef VKE_RENDERER_H
#define VKE_RENDERER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>

namespace vke {

  class CommandBuffer;
  class Light;
  class LogicalDevice;
  class RenderPass;
  class RenderTarget;
  class SwapChain;

  class Renderer {
  public:
    explicit Renderer(std::shared_ptr<LogicalDevice> logicalDevice,
                      const std::shared_ptr<SwapChain>& swapChain,
                      VkCommandPool commandPool);

    virtual ~Renderer();

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getSwapchainRenderPass() const = 0;

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getOffscreenRenderPass() const = 0;

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getShadowRenderPass() const = 0;

    [[nodiscard]] virtual std::shared_ptr<RenderPass> getShadowCubeRenderPass() const = 0;

    [[nodiscard]] virtual VkDescriptorSet getOffscreenImageDescriptorSet(uint32_t imageIndex);

    virtual void resetSwapchainImageResources(const std::shared_ptr<SwapChain>& swapChain);

    virtual void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent);

    virtual void beginSwapchainRendering(uint32_t imageIndex,
                                         VkExtent2D extent,
                                         std::shared_ptr<CommandBuffer> commandBuffer,
                                         std::shared_ptr<SwapChain> swapChain) = 0;

    virtual void beginOffscreenRendering(uint32_t imageIndex,
                                         VkExtent2D extent,
                                         std::shared_ptr<CommandBuffer> commandBuffer) = 0;

    virtual void beginShadowRendering(uint32_t imageIndex,
                                      VkExtent2D extent,
                                      const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const std::shared_ptr<Light>& light) = 0;

    virtual void endSwapchainRendering(uint32_t imageIndex,
                                       std::shared_ptr<CommandBuffer> commandBuffer,
                                       std::shared_ptr<SwapChain> swapChain) = 0;

    virtual void endOffscreenRendering(uint32_t imageIndex,
                                       std::shared_ptr<CommandBuffer> commandBuffer) = 0;

    virtual void endShadowRendering(uint32_t imageIndex,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer) = 0;

    [[nodiscard]] virtual uint32_t registerShadowMapRenderTarget(std::shared_ptr<RenderTarget> renderTarget,
                                                                 bool isCubeMap);

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::shared_ptr<RenderTarget> m_offscreenRenderTarget;

    std::shared_ptr<RenderTarget> m_swapchainRenderTarget;

    VkSampler m_sampler = VK_NULL_HANDLE;

    uint32_t m_currentShadowMapRenderTargetID = 0;

    void createSampler();

    void createSwapchainRenderTarget(const std::shared_ptr<SwapChain>& swapChain);

    void createOffscreenRenderTarget(VkExtent2D extent);
  };

} // namespace vke

#endif //VKE_RENDERER_H
