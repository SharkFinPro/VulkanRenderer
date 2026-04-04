#ifndef VKE_SWAPCHAIN_H
#define VKE_SWAPCHAIN_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;
  class Surface;
  class Window;

  class SwapChain {
  public:
    SwapChain(std::shared_ptr<LogicalDevice> logicalDevice,
              const std::shared_ptr<Window>& window,
              const std::shared_ptr<Surface>& surface);

    [[nodiscard]] vk::Format getImageFormat() const;

    [[nodiscard]] vk::Extent2D getExtent() const;

    [[nodiscard]] vk::SwapchainKHR getSwapChain() const;

    [[nodiscard]] const std::vector<vk::raii::ImageView>& getImageViews() const;

    [[nodiscard]] const std::vector<vk::Image>& getImages() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::SwapchainKHR m_swapchain = nullptr;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D m_swapChainExtent{};
    std::vector<vk::raii::ImageView> m_swapChainImageViews;

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

    [[nodiscard]] static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                                       const std::shared_ptr<Window>& window);

    static uint32_t chooseSwapImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);

    void createSwapChain(const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Surface>& surface);

    void createImageViews();
  };

} // namespace vke

#endif //VKE_SWAPCHAIN_H
