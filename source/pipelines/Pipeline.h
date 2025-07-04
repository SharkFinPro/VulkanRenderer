#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class LogicalDevice;
class PhysicalDevice;

class Pipeline {
public:
  Pipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual ~Pipeline();

  virtual void displayGui() {}

protected:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};



#endif //PIPELINE_H
