#include "Pipeline.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {

  Pipeline::Pipeline(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {}

  void Pipeline::pushConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                               const vk::ShaderStageFlags stageFlags,
                               const uint32_t offset,
                               const uint32_t size,
                               const void* values) const
  {
    commandBuffer->pushConstants(m_pipelineLayout, stageFlags, offset, size, values);
  }

} // namespace vke