#ifndef VULKANPROJECT_LOGICALDEVICE_H
#define VULKANPROJECT_LOGICALDEVICE_H

#include <vulkan/vulkan.h>
#include <memory>

class PhysicalDevice;

class LogicalDevice {
public:
  explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);
  ~LogicalDevice();

  VkDevice& getDevice();
  void waitIdle() const;

  VkQueue& getGraphicsQueue();
  VkQueue& getPresentQueue();

private:
  VkDevice device;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
};


#endif //VULKANPROJECT_LOGICALDEVICE_H
