#ifndef VULKANPROJECT_SWAPCHAIN_H
#define VULKANPROJECT_SWAPCHAIN_H

#include "../core/physicalDevice/PhysicalDevice.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "Window.h"
#include <vulkan/vulkan.h>
#include <vector>

class SwapChain {
public:
  SwapChain(const std::shared_ptr<LogicalDevice>& logicalDevice, const std::shared_ptr<Window>& window);
  ~SwapChain();

  [[nodiscard]] VkFormat& getImageFormat();
  [[nodiscard]] VkExtent2D& getExtent();
  [[nodiscard]] VkSwapchainKHR& getSwapChain();

  [[nodiscard]] std::vector<VkImageView>& getImageViews();

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;
  std::shared_ptr<Window> m_window;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> m_swapChainImages;
  VkFormat m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D m_swapChainExtent{};
  std::vector<VkImageView> m_swapChainImageViews;

  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
  static uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
  void createSwapChain();
  void createImageViews();
};


#endif //VULKANPROJECT_SWAPCHAIN_H
