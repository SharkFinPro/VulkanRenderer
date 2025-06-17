#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include "Pipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

class ShaderModule;

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(const std::shared_ptr<PhysicalDevice> &physicalDevice, const std::shared_ptr<LogicalDevice> &logicalDevice);

protected:
  std::unique_ptr<ShaderModule> shaderModule;

  void createShader(const char* filename);

  virtual void loadComputeShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

  virtual void loadComputeDescriptorSetLayouts() {}

  void createPipelineLayout();

  void createPipeline();
};



#endif //COMPUTEPIPELINE_H
