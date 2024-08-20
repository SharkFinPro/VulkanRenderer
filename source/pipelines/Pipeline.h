#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

class Pipeline {
public:
  Pipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice);

  virtual ~Pipeline();

protected:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};



#endif //PIPELINE_H
