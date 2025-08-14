#ifndef COMPUTINGMANAGER_H
#define COMPUTINGMANAGER_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class CommandBuffer;
class LogicalDevice;
class PipelineManager;

class ComputingManager {
public:
  ComputingManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

  void doComputing(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t currentFrame) const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<CommandBuffer> m_computeCommandBuffer;

  void recordComputeCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager, uint32_t currentFrame) const;
};

} // namespace vke

#endif //COMPUTINGMANAGER_H
