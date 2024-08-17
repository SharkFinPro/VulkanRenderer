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
  VkQueue& getComputeQueue();

  void submitGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitComputeQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;

  void waitForGraphicsFences(uint32_t currentFrame) const;
  void waitForComputeFences(uint32_t currentFrame) const;
  void resetGraphicsFences(uint32_t currentFrame) const;
  void resetComputeFences(uint32_t currentFrame) const;

  VkResult queuePresent(uint32_t currentFrame, VkSwapchainKHR& swapchain, uint32_t* imageIndex);

  VkResult acquireNextImage(uint32_t currentFrame, const VkSwapchainKHR& swapchain, uint32_t* imageIndex);

private:
  void createDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

  void createSyncObjects();

private:
  VkDevice device;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue computeQueue;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkFence> computeInFlightFences;
};


#endif //VULKANPROJECT_LOGICALDEVICE_H
