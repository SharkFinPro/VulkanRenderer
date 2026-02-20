#ifndef VULKANPROJECT_RAYTRACINGPIPELINE_H
#define VULKANPROJECT_RAYTRACINGPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vke {

  struct RayTracingPipelineConfig {
    struct {
      std::string rayGenerationShader;
      std::string missShader;
      std::string closestHitShader;

      [[nodiscard]] std::vector<ShaderModule> getShaderModules(const std::shared_ptr<LogicalDevice>& logicalDevice) const
      {
        if (rayGenerationShader.empty() ||
            missShader.empty() ||
            closestHitShader.empty())
        {
          throw std::runtime_error("Missing required ray tracing shaders!");
        }

        std::vector<ShaderModule> shaderModules;

        shaderModules.emplace_back(logicalDevice, rayGenerationShader.c_str(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        shaderModules.emplace_back(logicalDevice, missShader.c_str(), VK_SHADER_STAGE_MISS_BIT_KHR);

        shaderModules.emplace_back(logicalDevice, closestHitShader.c_str(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        return std::move(shaderModules);
      }

      static std::vector<VkPipelineShaderStageCreateInfo> getShaderStages(const std::vector<ShaderModule>& shaderModules)
      {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        for (const auto& shaderModule : shaderModules)
        {
          pipelineShaderStageCreateInfos.push_back(shaderModule.getShaderStageCreateInfo());
        }

        return std::move(pipelineShaderStageCreateInfos);
      }
    } shaders;

    std::vector<VkPushConstantRange> pushConstantRanges;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  };

  class RayTracingPipeline : public Pipeline {
  public:
    RayTracingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                       const RayTracingPipelineConfig& config);

  protected:
    void createPipelineLayout(const RayTracingPipelineConfig& config);

    void createPipeline(const RayTracingPipelineConfig& config);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACINGPIPELINE_H