#ifndef VKE_FRAMEBUFFER_H
#define VKE_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;
  class RenderPass;
  class RenderTarget;
  class SwapChain;

  class Framebuffer final {
  public:
    explicit Framebuffer(std::shared_ptr<LogicalDevice> logicalDevice,
                         const std::shared_ptr<RenderTarget>& renderTarget,
                         const std::shared_ptr<RenderPass>& renderPass,
                         VkExtent2D extent,
                         const std::shared_ptr<SwapChain>& swapChain = nullptr);

    ~Framebuffer();

    VkFramebuffer& getFramebuffer(uint32_t imageIndex);

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<VkFramebuffer> m_framebuffers;

    void createFrameBuffers(const VkRenderPass& renderPass,
                            VkExtent2D extent,
                            const std::shared_ptr<RenderTarget>& renderTarget,
                            const std::shared_ptr<SwapChain>& swapChain);
  };

} // namespace vke

#endif //VKE_FRAMEBUFFER_H
