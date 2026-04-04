#ifndef VKE_FRAMEBUFFER_H
#define VKE_FRAMEBUFFER_H

#include <vulkan/vulkan_raii.hpp>
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
                         vk::Extent2D extent,
                         const std::shared_ptr<SwapChain>& swapChain = nullptr);

    [[nodiscard]] const vk::raii::Framebuffer& getFramebuffer(uint32_t imageIndex) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<vk::raii::Framebuffer> m_framebuffers;

    void createFrameBuffers(const vk::raii::RenderPass& renderPass,
                            vk::Extent2D extent,
                            const std::shared_ptr<RenderTarget>& renderTarget,
                            const std::shared_ptr<SwapChain>& swapChain);
  };

} // namespace vke

#endif //VKE_FRAMEBUFFER_H
