#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include <functional>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class LogicalDevice;

class CommandBuffer {
public:
  CommandBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

  void record(uint32_t currentFrame, const std::function<void(const VkCommandBuffer& cmdBuffer)>& renderFunction) const;

  void resetCommandBuffer(uint32_t currentFrame) const;

  [[nodiscard]] VkCommandBuffer* getCommandBuffer(uint32_t currentFrame);

private:
  std::shared_ptr<LogicalDevice> logicalDevice;

  std::vector<VkCommandBuffer> commandBuffers;

  void allocateCommandBuffers(VkCommandPool commandPool);
};



#endif //COMMANDBUFFER_H
