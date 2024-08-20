#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "ShaderModule.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

class ComputePipeline {
public:
  ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice);

  virtual ~ComputePipeline();

protected:
  void createShader(const char* filename);

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

  virtual void loadDescriptorSetLayouts() {};

  void createPipelineLayout();

  void createPipeline();

protected:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

  std::unique_ptr<ShaderModule> shaderModule;
};



#endif //COMPUTEPIPELINE_H
