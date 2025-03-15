#ifndef VULKANPROJECT_LOGICALDEVICE_H
#define VULKANPROJECT_LOGICALDEVICE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class PhysicalDevice;

class LogicalDevice {
public:
  explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);
  ~LogicalDevice();

  [[nodiscard]] VkDevice getDevice() const;
  void waitIdle() const;

  [[nodiscard]] VkQueue getGraphicsQueue() const;
  [[nodiscard]] VkQueue getPresentQueue() const;
  [[nodiscard]] VkQueue getComputeQueue() const;

  void submitOffscreenGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitComputeQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;

  void waitForGraphicsFences(uint32_t currentFrame) const;
  void waitForComputeFences(uint32_t currentFrame) const;
  void resetGraphicsFences(uint32_t currentFrame) const;
  void resetComputeFences(uint32_t currentFrame) const;

  VkResult queuePresent(uint32_t currentFrame, const VkSwapchainKHR& swapchain, const uint32_t* imageIndex) const;

  VkResult acquireNextImage(uint32_t currentFrame, const VkSwapchainKHR& swapchain, uint32_t* imageIndex) const;

  [[nodiscard]] uint32_t getMaxFramesInFlight() const;

private:
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkQueue computeQueue = VK_NULL_HANDLE;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> imageAvailableSemaphores2;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores2;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> inFlightFences2;

  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkFence> computeInFlightFences;

  uint8_t maxFramesInFlight = 2;

  void createDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

  void createSyncObjects();
};


#endif //VULKANPROJECT_LOGICALDEVICE_H
