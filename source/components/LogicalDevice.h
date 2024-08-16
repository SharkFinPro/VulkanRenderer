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

  VkDevice& getDevice();
  void waitIdle() const;

  VkQueue& getGraphicsQueue();
  VkQueue& getPresentQueue();

  void submitGraphicsQueue(uint32_t currentFrame, VkCommandBuffer* commandBuffer);

  void waitForFences(uint32_t currentFrame);
  void resetFences(uint32_t currentFrame);

  VkResult queuePresent(uint32_t currentFrame, VkSwapchainKHR& swapchain, uint32_t* imageIndex);

  VkResult acquireNextImage(uint32_t currentFrame, VkSwapchainKHR& swapchain, uint32_t* imageIndex);

private:
  void createDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

  void createSyncObjects();

private:
  VkDevice device;

  VkQueue computeQueue;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
};


#endif //VULKANPROJECT_LOGICALDEVICE_H
