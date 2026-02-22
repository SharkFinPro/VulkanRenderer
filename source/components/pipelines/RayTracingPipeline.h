#ifndef VULKANPROJECT_RAYTRACINGPIPELINE_H
#define VULKANPROJECT_RAYTRACINGPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vke {

  class DescriptorSet;
  struct RenderInfo;

  struct RayTracingPipelineConfig {
    struct {
      std::string rayGenerationShader;
      std::vector<std::string> missShaders;
      std::string closestHitShader;

      [[nodiscard]] std::vector<ShaderModule> getShaderModules(const std::shared_ptr<LogicalDevice>& logicalDevice) const
      {
        if (rayGenerationShader.empty() ||
            missShaders.empty() ||
            closestHitShader.empty())
        {
          throw std::runtime_error("Missing required ray tracing shaders!");
        }

        std::vector<ShaderModule> shaderModules;

        shaderModules.emplace_back(logicalDevice, rayGenerationShader.c_str(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        for (const auto& missShader : missShaders)
        {
          shaderModules.emplace_back(logicalDevice, missShader.c_str(), VK_SHADER_STAGE_MISS_BIT_KHR);
        }

        shaderModules.emplace_back(logicalDevice, closestHitShader.c_str(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        return shaderModules;
      }

      static std::vector<VkPipelineShaderStageCreateInfo> getShaderStages(const std::vector<ShaderModule>& shaderModules)
      {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        for (const auto& shaderModule : shaderModules)
        {
          pipelineShaderStageCreateInfos.push_back(shaderModule.getShaderStageCreateInfo());
        }

        return pipelineShaderStageCreateInfos;
      }
    } shaders;

    std::vector<VkPushConstantRange> pushConstantRanges;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  };

  class RayTracingPipeline : public Pipeline {
  public:
    RayTracingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                       const RayTracingPipelineConfig& config);

    ~RayTracingPipeline() override;

    [[nodiscard]] VkStridedDeviceAddressRegionKHR getRayGenerationRegion() const { return m_rayGenerationRegion; }
    [[nodiscard]] VkStridedDeviceAddressRegionKHR getMissRegion() const { return m_missRegion; }
    [[nodiscard]] VkStridedDeviceAddressRegionKHR getHitRegion() const { return m_hitRegion; }
    [[nodiscard]] VkStridedDeviceAddressRegionKHR getCallableRegion() const { return m_callableRegion; }

    void doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                      VkExtent2D extent) const;

    void bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                           VkDescriptorSet descriptorSet,
                           uint32_t location) const;

  protected:
    VkBuffer m_shaderBindingTableBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_shaderBindingTableMemory = VK_NULL_HANDLE;

    VkStridedDeviceAddressRegionKHR m_rayGenerationRegion{};
    VkStridedDeviceAddressRegionKHR m_missRegion{};
    VkStridedDeviceAddressRegionKHR m_hitRegion{};
    VkStridedDeviceAddressRegionKHR m_callableRegion{};

    void createPipelineLayout(const RayTracingPipelineConfig& config);

    void createPipeline(const RayTracingPipelineConfig& config);

    void createShaderBindingTable(const RayTracingPipelineConfig& config);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACINGPIPELINE_H