#ifndef VULKANPROJECT_SWAPCHAIN_H
#define VULKANPROJECT_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

#include "PhysicalDevice.h"
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
  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  static uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
  void createSwapChain();
  void createImageViews();

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<Window> window;

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
};


#endif //VULKANPROJECT_SWAPCHAIN_H
