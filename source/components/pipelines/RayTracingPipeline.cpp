#include "RayTracingPipeline.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../../utilities/Buffers.h"
#include <cstring>

namespace vke {
  RayTracingPipeline::RayTracingPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                                         const RayTracingPipelineConfig& config)
    : Pipeline(std::move(logicalDevice))
  {
    createPipeline(config);

    createShaderBindingTable(config);
  }

  void RayTracingPipeline::doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                        const vk::Extent2D extent) const
  {
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);

    commandBuffer->traceRays(
      m_rayGenerationRegion,
      m_missRegion,
      m_hitRegion,
      m_callableRegion,
      extent.width,
      extent.height,
      1
    );
  }

  void RayTracingPipeline::bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             const vk::DescriptorSet descriptorSet,
                                             const uint32_t location) const
  {
    commandBuffer->bindDescriptorSets(
      vk::PipelineBindPoint::eRayTracingKHR,
      m_pipelineLayout,
      location,
      { descriptorSet }
    );
  }

  void RayTracingPipeline::createPipelineLayout(const RayTracingPipelineConfig& config)
  {
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
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

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;

    groups.push_back({
      .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
      .generalShader = 0,
      .closestHitShader = vk::ShaderUnusedKHR,
      .anyHitShader = vk::ShaderUnusedKHR,
      .intersectionShader = vk::ShaderUnusedKHR
    });

    for (const auto& _ : config.shaders.missShaders)
    {
      groups.push_back({
        .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
        .generalShader = static_cast<uint32_t>(groups.size()),
        .closestHitShader = vk::ShaderUnusedKHR,
        .anyHitShader = vk::ShaderUnusedKHR,
        .intersectionShader = vk::ShaderUnusedKHR
      });
    }

    uint32_t stageIndex = static_cast<uint32_t>(groups.size());

    for (const auto& hitGroup : config.shaders.hitGroups)
    {
      const bool isProcedural = !hitGroup.intersectionShader.empty();

      groups.push_back({
        .type = isProcedural ? vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup : vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
        .generalShader = vk::ShaderUnusedKHR,
        .closestHitShader = stageIndex++,
        .anyHitShader = vk::ShaderUnusedKHR,
        .intersectionShader = isProcedural ? stageIndex++ : vk::ShaderUnusedKHR
      });
    }

    const vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo {
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .groupCount = static_cast<uint32_t>(groups.size()),
      .pGroups = groups.data(),
      .maxPipelineRayRecursionDepth = 5,
      .layout = *m_pipelineLayout,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1
    };

    m_pipeline = m_logicalDevice->createPipeline(rayTracingPipelineCreateInfo);
  }

  void RayTracingPipeline::createShaderBindingTable(const RayTracingPipelineConfig& config)
  {
    const auto rayTracingPipelineProperties = m_logicalDevice->getPhysicalDevice()->getRayTracingPipelineProperties();

    const uint32_t shaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t shaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;

    const uint32_t alignedHandleSize = (shaderGroupHandleSize + shaderGroupHandleAlignment - 1) & ~(shaderGroupHandleAlignment - 1);

    const auto missCount = static_cast<uint32_t>(config.shaders.missShaders.size());
    const auto hitGroupCount = static_cast<uint32_t>(config.shaders.hitGroups.size());
    const uint32_t groupCount = 1 + missCount + hitGroupCount;

    const uint32_t shaderBindingTableSize = groupCount * shaderGroupBaseAlignment;

    std::vector<uint8_t> handles(groupCount * shaderGroupHandleSize);

    m_logicalDevice->getRayTracingShaderGroupHandles(m_pipeline, groupCount, handles);

    Buffers::createBuffer(
      m_logicalDevice,
      shaderBindingTableSize,
      vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      m_shaderBindingTableBuffer,
      m_shaderBindingTableMemory
    );

    m_logicalDevice->doMappedMemoryOperation(m_shaderBindingTableMemory, [groupCount, handles, shaderGroupBaseAlignment, shaderGroupHandleSize](void* data) {
      auto* dst = static_cast<uint8_t*>(data);
      for (uint32_t i = 0; i < groupCount; ++i)
      {
        memcpy(dst + i * shaderGroupBaseAlignment, handles.data() + i * shaderGroupHandleSize, shaderGroupHandleSize);
      }
    });

    const auto shaderBindingTableAddress = m_logicalDevice->getBufferDeviceAddress(m_shaderBindingTableBuffer);

    m_rayGenerationRegion = vk::StridedDeviceAddressRegionKHR{
      .deviceAddress = shaderBindingTableAddress,
      .stride = shaderGroupBaseAlignment,
      .size = shaderGroupBaseAlignment
    };

    m_missRegion = vk::StridedDeviceAddressRegionKHR{
      .deviceAddress = shaderBindingTableAddress + shaderGroupBaseAlignment,
      .stride = shaderGroupBaseAlignment,
      .size = shaderGroupBaseAlignment * missCount
    };

    m_hitRegion = vk::StridedDeviceAddressRegionKHR{
      .deviceAddress = shaderBindingTableAddress + shaderGroupBaseAlignment * (1 + missCount),
      .stride = shaderGroupBaseAlignment,
      .size = shaderGroupBaseAlignment * hitGroupCount
    };

    m_callableRegion = vk::StridedDeviceAddressRegionKHR{};
  }

} // namespace vke