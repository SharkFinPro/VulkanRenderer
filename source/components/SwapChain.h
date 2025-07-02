#ifndef VULKANPROJECT_SWAPCHAIN_H
#define VULKANPROJECT_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

#include "../core/physicalDevice/PhysicalDevice.h"
#include "LogicalDevice.h"
#include "Window.h"

class SwapChain {
public:
  SwapChain(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
            const std::shared_ptr<Window>& window);
  ~SwapChain();

  [[nodiscard]] VkFormat& getImageFormat();
  [[nodiscard]] VkExtent2D& getExtent();
  [[nodiscard]] VkSwapchainKHR& getSwapChain();

  [[nodiscard]] std::vector<VkImageView>& getImageViews();

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<Window> window;

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D swapChainExtent{};
  std::vector<VkImageView> swapChainImageViews;

  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
  static uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
  void createSwapChain();
  void createImageViews();
};


#endif //VULKANPROJECT_SWAPCHAIN_H
