#ifndef VKE_SWAPCHAIN_H
#define VKE_SWAPCHAIN_H

#include <vulkan/vulkan.h>
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

    ~SwapChain();

    [[nodiscard]] VkFormat& getImageFormat();
    [[nodiscard]] VkExtent2D& getExtent();
    [[nodiscard]] VkSwapchainKHR& getSwapChain();

    [[nodiscard]] std::vector<VkImageView>& getImageViews();

    [[nodiscard]] std::vector<VkImage>& getImages();

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapChainExtent{};
    std::vector<VkImageView> m_swapChainImageViews;

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    [[nodiscard]] static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                                     const std::shared_ptr<Window>& window);

    static uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

    void createSwapChain(const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Surface>& surface);

    void createImageViews();
  };

} // namespace vke

#endif //VKE_SWAPCHAIN_H
