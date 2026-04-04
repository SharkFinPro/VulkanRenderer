#ifndef VKE_COMPUTEPIPELINE_H
#define VKE_COMPUTEPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <vulkan/vulkan_raii.hpp>
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

        return ShaderModule(logicalDevice, computeShader.c_str(), vk::ShaderStageFlagBits::eCompute);
      }
    } shaders;

    std::vector<vk::PushConstantRange> pushConstantRanges;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
  };

  class ComputePipeline : public Pipeline {
  public:
    explicit ComputePipeline(std::shared_ptr<LogicalDevice> logicalDevice);

  protected:
    void createPipelineLayout(const ComputePipelineOptions& computePipelineOptions);

    void createPipeline(const ComputePipelineOptions& computePipelineOptions);
  };

} // namespace vke

#endif //VKE_COMPUTEPIPELINE_H