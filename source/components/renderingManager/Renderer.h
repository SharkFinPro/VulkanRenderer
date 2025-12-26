#ifndef VKE_RENDERER_H
#define VKE_RENDERER_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class CommandBuffer;
class Light;
class LogicalDevice;
class RenderPass;
class SwapChain;

class Renderer {
public:
  explicit Renderer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

  virtual ~Renderer() = default;

  [[nodiscard]] virtual std::shared_ptr<RenderPass> getRenderPass() const = 0;

  [[nodiscard]] virtual VkDescriptorSet getOffscreenImageDescriptorSet(uint32_t imageIndex) = 0;

  virtual void resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain) = 0;

  virtual void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) = 0;

  virtual void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                                       std::shared_ptr<CommandBuffer> commandBuffer,
                                       std::shared_ptr<SwapChain> swapChain) = 0;

  virtual void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                                       std::shared_ptr<CommandBuffer> commandBuffer) = 0;

  virtual void beginShadowRendering(uint32_t imageIndex,
                                    VkExtent2D extent,
                                    const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<Light>& light) = 0;

  virtual void endSwapchainRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer,
                                     std::shared_ptr<SwapChain> swapChain) = 0;

  virtual void endOffscreenRendering(uint32_t imageIndex, std::shared_ptr<CommandBuffer> commandBuffer) = 0;

  virtual void endShadowRendering(uint32_t imageIndex,
                                  const std::shared_ptr<CommandBuffer>& commandBuffer) = 0;

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vke

#endif //VKE_RENDERER_H
