#ifndef VKE_SWAPCHAIN_H
#define VKE_SWAPCHAIN_H

#include "PresentMode.h"
#include "../renderingManager/ImageResource.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class Surface;
  class Window;

  class SwapChain {
  public:
    SwapChain(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const std::shared_ptr<Window>& window,
              const std::shared_ptr<Surface>& surface,
              vk::CommandPool commandPool,
              PresentMode preference,
              vk::SwapchainKHR oldSwapchain = nullptr);

    // The mode in use, which differs from the requested preference when the surface lacks it.
    [[nodiscard]] PresentMode getPresentMode() const;

    [[nodiscard]] vk::Format getImageFormat() const;

    [[nodiscard]] vk::Extent2D getExtent() const;

    [[nodiscard]] vk::SwapchainKHR getSwapChain() const;

    [[nodiscard]] const std::vector<vk::raii::ImageView>& getImageViews() const;

    [[nodiscard]] const std::vector<vk::Image>& getImages() const;

    void beginRendering(uint32_t imageIndex,
                        const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void endRendering(uint32_t imageIndex,
                      const std::shared_ptr<CommandBuffer>& commandBuffer) const;

  private:
    vk::raii::SwapchainKHR m_swapchain = nullptr;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D m_swapChainExtent{};
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;

    std::vector<ImageResource> m_colorImageResources;
    std::vector<ImageResource> m_depthImageResources;

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    [[nodiscard]] static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                                       const std::shared_ptr<Window>& window);

    static uint32_t chooseSwapImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);

    void createSwapChain(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Surface>& surface,
                         PresentMode preference,
                         vk::SwapchainKHR oldSwapchain);

    void createImageViews(const std::shared_ptr<LogicalDevice>& logicalDevice);

    void createImageResources(const std::shared_ptr<LogicalDevice>& logicalDevice,
                              vk::CommandPool commandPool);

    static void transitionImagePreRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                         vk::Image image);

    static void transitionImagePostRender(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                          vk::Image image);
  };

} // namespace vke

#endif //VKE_SWAPCHAIN_H
