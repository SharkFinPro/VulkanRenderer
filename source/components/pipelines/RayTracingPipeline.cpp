#include "RayTracingPipeline.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {
  RayTracingPipeline::RayTracingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                         const RayTracingPipelineConfig& config)
    : Pipeline(std::move(logicalDevice))
  {
    createPipeline(config);
  }

  void RayTracingPipeline::createPipelineLayout(const RayTracingPipelineConfig& config)
  {
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size()),
      .pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size()),
      .pPushConstantRanges = config.pushConstantRanges.empty() ? nullptr : config.pushConstantRanges.data()
    };

    m_pipelineLayout = m_logicalDevice->createPipelineLayout(pipelineLayoutInfo);
  }

  void RayTracingPipeline::createPipeline(const RayTracingPipelineConfig& config)
  {
    createPipelineLayout(config);

    const auto shaderModules = config.shaders.getShaderModules(m_logicalDevice);
    const auto shaderStages = config.shaders.getShaderStages(shaderModules);

    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups {
      {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader = 0,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR
      },
      {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader = 1,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR
      },
      {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = 2,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR
      }
    };

    const VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo {
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .groupCount = static_cast<uint32_t>(groups.size()),
      .pGroups = groups.data(),
      .maxPipelineRayRecursionDepth = 1,
      .layout = m_pipelineLayout,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1
    };

    m_pipeline = m_logicalDevice->createPipeline(rayTracingPipelineCreateInfo);
  }
} // vke