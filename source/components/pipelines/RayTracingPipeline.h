#ifndef VULKANPROJECT_RAYTRACINGPIPELINE_H
#define VULKANPROJECT_RAYTRACINGPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace vke {

  class DescriptorSet;
  struct RenderInfo;

  struct RayTracingPipelineConfig {
    struct HitGroup {
      std::string closestHitShader;
      std::string intersectionShader;
    };

    struct {
      std::string rayGenerationShader;
      std::vector<std::string> missShaders;
      std::vector<HitGroup> hitGroups;

      [[nodiscard]] std::vector<ShaderModule> getShaderModules(const std::shared_ptr<LogicalDevice>& logicalDevice) const
      {
        if (rayGenerationShader.empty() ||
            missShaders.empty() ||
            hitGroups.empty())
        {
          throw std::runtime_error("Missing required ray tracing shaders!");
        }

        std::vector<ShaderModule> shaderModules;

        shaderModules.emplace_back(logicalDevice, rayGenerationShader.c_str(), vk::ShaderStageFlagBits::eRaygenKHR);

        for (const auto& missShader : missShaders)
        {
          shaderModules.emplace_back(logicalDevice, missShader.c_str(), vk::ShaderStageFlagBits::eMissKHR);
        }

        for (const auto& hitGroup : hitGroups)
        {
          shaderModules.emplace_back(logicalDevice, hitGroup.closestHitShader.c_str(), vk::ShaderStageFlagBits::eClosestHitKHR);

          if (!hitGroup.intersectionShader.empty())
          {
            shaderModules.emplace_back(logicalDevice, hitGroup.intersectionShader.c_str(), vk::ShaderStageFlagBits::eIntersectionKHR);
          }
        }

        return shaderModules;
      }

      static std::vector<vk::PipelineShaderStageCreateInfo> getShaderStages(const std::vector<ShaderModule>& shaderModules)
      {
        std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        for (const auto& shaderModule : shaderModules)
        {
          pipelineShaderStageCreateInfos.push_back(shaderModule.getShaderStageCreateInfo());
        }

        return pipelineShaderStageCreateInfos;
      }
    } shaders;

    std::vector<vk::PushConstantRange> pushConstantRanges;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
  };

  class RayTracingPipeline : public Pipeline {
  public:
    RayTracingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                       const RayTracingPipelineConfig& config);

    ~RayTracingPipeline() override = default;

    [[nodiscard]] vk::StridedDeviceAddressRegionKHR getRayGenerationRegion() const { return m_rayGenerationRegion; }
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR getMissRegion() const { return m_missRegion; }
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR getHitRegion() const { return m_hitRegion; }
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR getCallableRegion() const { return m_callableRegion; }

    void doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                      vk::Extent2D extent) const;

    void bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                           vk::DescriptorSet descriptorSet,
                           uint32_t location) const;

  protected:
    vk::raii::Buffer m_shaderBindingTableBuffer = nullptr;
    vk::raii::DeviceMemory m_shaderBindingTableMemory = nullptr;

    vk::StridedDeviceAddressRegionKHR m_rayGenerationRegion{};
    vk::StridedDeviceAddressRegionKHR m_missRegion{};
    vk::StridedDeviceAddressRegionKHR m_hitRegion{};
    vk::StridedDeviceAddressRegionKHR m_callableRegion{};

    void createPipelineLayout(const RayTracingPipelineConfig& config);

    void createPipeline(const RayTracingPipelineConfig& config);

    void createShaderBindingTable(const RayTracingPipelineConfig& config);
  };

} // vke

#endif //VULKANPROJECT_RAYTRACINGPIPELINE_H