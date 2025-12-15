#ifndef VKE_COMPUTEPIPELINE_H
#define VKE_COMPUTEPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <vulkan/vulkan.h>
#include <cassert>
#include <memory>
#include <string>

namespace vke {

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

} // namespace vke

#endif //VKE_COMPUTEPIPELINE_H
