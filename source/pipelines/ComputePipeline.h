#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include "Pipeline.h"
#include "ShaderModule.h"
#include <vulkan/vulkan.h>
#include <cassert>
#include <memory>
#include <string>

struct ComputePipelineOptions {
  struct {
    std::string computeShader;

    [[nodiscard]] ShaderModule getShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice) const
    {
      assert(!computeShader.empty());

      auto shaderModule = ShaderModule(logicalDevice, computeShader.c_str(), VK_SHADER_STAGE_COMPUTE_BIT);

      return std::move(shaderModule);
    }
  } shaders;

  std::vector<VkPushConstantRange> pushConstantRanges;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

class ComputePipeline : public Pipeline {
public:
  explicit ComputePipeline(const std::shared_ptr<LogicalDevice>& logicalDevice);

protected:
  void createPipelineLayout(const ComputePipelineOptions& computePipelineOptions);

  void createPipeline(const ComputePipelineOptions& computePipelineOptions);
};



#endif //COMPUTEPIPELINE_H
