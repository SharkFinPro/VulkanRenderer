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

  RayTracingPipeline::~RayTracingPipeline()
  {
    Buffers::destroyBuffer(m_logicalDevice, m_shaderBindingTableBuffer, m_shaderBindingTableMemory);
  }

  void RayTracingPipeline::doRayTracing(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                        const VkExtent2D extent) const
  {
    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);

    commandBuffer->traceRays(
      &m_rayGenerationRegion,
      &m_missRegion,
      &m_hitRegion,
      &m_callableRegion,
      extent.width,
      extent.height,
      1
    );
  }

  void RayTracingPipeline::bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                             VkDescriptorSet descriptorSet,
                                             const uint32_t location) const
  {
    commandBuffer->bindDescriptorSets(
      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
      m_pipelineLayout,
      location,
      1,
      &descriptorSet
    );
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

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

    groups.push_back({
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
      .generalShader = static_cast<uint32_t>(groups.size()),
      .closestHitShader = VK_SHADER_UNUSED_KHR,
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR
    });

    for (const auto& _ : config.shaders.missShaders)
    {
      groups.push_back({
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader = static_cast<uint32_t>(groups.size()),
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR
      });
    }

    groups.push_back({
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
      .generalShader = VK_SHADER_UNUSED_KHR,
      .closestHitShader = static_cast<uint32_t>(groups.size()),
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR
    });

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

  void RayTracingPipeline::createShaderBindingTable(const RayTracingPipelineConfig& config)
  {
    const auto rayTracingPipelineProperties = m_logicalDevice->getPhysicalDevice()->getRayTracingPipelineProperties();

    const uint32_t shaderGroupHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t shaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;

    const uint32_t alignedHandleSize = (shaderGroupHandleSize + shaderGroupHandleAlignment - 1) & ~(shaderGroupHandleAlignment - 1);

    const auto missCount = static_cast<uint32_t>(config.shaders.missShaders.size());

    const uint32_t groupCount = 1 + missCount + 1;

    const uint32_t shaderBindingTableSize = groupCount * shaderGroupBaseAlignment;

    std::vector<uint8_t> handles(groupCount * shaderGroupHandleSize);

    m_logicalDevice->getRayTracingShaderGroupHandles(m_pipeline, groupCount, handles);

    Buffers::createBuffer(
      m_logicalDevice,
      shaderBindingTableSize,
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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

    m_rayGenerationRegion = {
      .deviceAddress = shaderBindingTableAddress,
      .stride = shaderGroupBaseAlignment,
      .size = shaderGroupBaseAlignment
    };

    m_missRegion = {
      .deviceAddress = shaderBindingTableAddress + shaderGroupBaseAlignment,
      .stride = shaderGroupBaseAlignment,
      .size = shaderGroupBaseAlignment * missCount
    };

    m_hitRegion = {
      .deviceAddress = shaderBindingTableAddress + shaderGroupBaseAlignment * (1 + missCount),
      .stride = alignedHandleSize,
      .size = alignedHandleSize
    };

    m_callableRegion = {};
  }
} // vke