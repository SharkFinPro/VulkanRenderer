#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>

#include "ShaderModule.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include "Pipeline.h"

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(const std::shared_ptr<PhysicalDevice> &physicalDevice, const std::shared_ptr<LogicalDevice> &logicalDevice);

protected:
  void createShader(const char* filename);

  virtual void loadComputeShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

  virtual void loadComputeDescriptorSetLayouts() {};

  void createPipelineLayout();

  void createPipeline();

protected:
  std::unique_ptr<ShaderModule> shaderModule;
};



#endif //COMPUTEPIPELINE_H
