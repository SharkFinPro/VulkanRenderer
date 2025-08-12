#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h>
#include <memory>

class CommandBuffer;
class LogicalDevice;
class RenderPass;
class SwapChain;

class Renderer {
public:
  explicit Renderer(const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual ~Renderer() = default;

  [[nodiscard]] virtual std::shared_ptr<RenderPass> getRenderPass() const = 0;

  [[nodiscard]] virtual VkDescriptorSet& getOffscreenImageDescriptorSet(uint32_t imageIndex) = 0;

  virtual void resetSwapchainImageResources(std::shared_ptr<SwapChain> swapChain) = 0;

  virtual void resetOffscreenImageResources(VkExtent2D offscreenViewportExtent) = 0;

  virtual void beginSwapchainRendering(uint32_t imageIndex, VkExtent2D extent,
                                       std::shared_ptr<CommandBuffer> commandBuffer) = 0;

  virtual void beginOffscreenRendering(uint32_t imageIndex, VkExtent2D extent,
                                       std::shared_ptr<CommandBuffer> commandBuffer) = 0;

  virtual void endRendering(std::shared_ptr<CommandBuffer> commandBuffer) = 0;

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;
};



#endif //RENDERER_H
